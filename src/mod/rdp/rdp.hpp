/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Product name: redemption, a FLOSS RDP proxy
  Copyright (C) Wallix 2010
  Author(s): Christophe Grosjean, Javier Caverni, Dominique Lafages,
             Raphael Zhou, Meng Tan
  Based on xrdp Copyright (C) Jay Sorg 2004-2010

  rdp module main header file
*/

#ifndef _REDEMPTION_MOD_RDP_RDP_HPP_
#define _REDEMPTION_MOD_RDP_RDP_HPP_

#include <queue>

#include "rdp/rdp_orders.hpp"

/* include "ther h files */
#include "stream.hpp"
#include "ssl_calls.hpp"
#include "mod_api.hpp"
#include "auth_api.hpp"
#include "front_api.hpp"

#include "RDP/x224.hpp"
#include "RDP/nego.hpp"
#include "RDP/mcs.hpp"
#include "RDP/lic.hpp"
#include "RDP/logon.hpp"
#include "channel_list.hpp"
#include "RDP/gcc.hpp"
#include "RDP/sec.hpp"
#include "colors.hpp"
#include "RDP/autoreconnect.hpp"
#include "RDP/ServerRedirection.hpp"
#include "RDP/bitmapupdate.hpp"
#include "RDP/clipboard.hpp"
#include "RDP/fastpath.hpp"
#include "RDP/PersistentKeyListPDU.hpp"
#include "RDP/protocol.hpp"
#include "RDP/RefreshRectPDU.hpp"
#include "RDP/SaveSessionInfoPDU.hpp"
#include "RDP/pointer.hpp"
#include "RDP/mppc_unified_dec.hpp"
#include "RDP/capabilities/cap_bitmap.hpp"
#include "RDP/capabilities/order.hpp"
#include "RDP/capabilities/bmpcache.hpp"
#include "RDP/capabilities/bmpcache2.hpp"
#include "RDP/capabilities/colcache.hpp"
#include "RDP/capabilities/activate.hpp"
#include "RDP/capabilities/control.hpp"
#include "RDP/capabilities/pointer.hpp"
#include "RDP/capabilities/cap_share.hpp"
#include "RDP/capabilities/input.hpp"
#include "RDP/capabilities/cap_sound.hpp"
#include "RDP/capabilities/cap_font.hpp"
#include "RDP/capabilities/glyphcache.hpp"
#include "RDP/capabilities/rail.hpp"
#include "RDP/capabilities/window.hpp"
#include "RDP/channels/rdpdr.hpp"
#include "RDP/remote_programs.hpp"
#include "rdp_params.hpp"
#include "transparentrecorder.hpp"
#include "FSCC/FileInformation.hpp"

#include "cast.hpp"
#include "client_info.hpp"
#include "genrandom.hpp"
#include "authorization_channels.hpp"
#include "parser.hpp"
#include "channel_names.hpp"
#include "finally.hpp"
#include "timeout.hpp"
#include "outbound_connection_monitor_rules.hpp"

#include "channels/cliprdr_channel.hpp"
#include "channels/rdpdr_channel.hpp"
#include "channels/rdpdr_file_system_drive_manager.hpp"

class RDPChannelManagerMod : public mod_api
{
private:
    std::unique_ptr<VirtualChannelDataSender> file_system_to_client_sender;
    std::unique_ptr<VirtualChannelDataSender> file_system_to_server_sender;

    std::unique_ptr<FileSystemVirtualChannel> file_system_virtual_channel;

    std::unique_ptr<VirtualChannelDataSender> clipboard_to_client_sender;
    std::unique_ptr<VirtualChannelDataSender> clipboard_to_server_sender;

    std::unique_ptr<ClipboardVirtualChannel>  clipboard_virtual_channel;

protected:
    FileSystemDriveManager file_system_drive_manager;

    FrontAPI& front;

    class ToClientSender : public VirtualChannelDataSender
    {
        FrontAPI& front;

        const CHANNELS::ChannelDef& channel;

        uint32_t verbose;

    public:
        ToClientSender(FrontAPI& front,
                       const CHANNELS::ChannelDef& channel,
                       uint32_t verbose)
        : front(front)
        , channel(channel)
        , verbose(verbose) {}

        void operator()(uint32_t total_length, uint32_t flags,
            const uint8_t* chunk_data, uint32_t chunk_data_length)
                override
        {
            if ((this->verbose & MODRDP_LOGLEVEL_CLIPRDR_DUMP) ||
                (this->verbose & MODRDP_LOGLEVEL_RDPDR_DUMP)) {
                const bool send              = true;
                const bool from_or_to_client = true;
                ::msgdump_c(send, from_or_to_client, total_length, flags,
                    chunk_data, chunk_data_length);
            }

            this->front.send_to_channel(this->channel,
                chunk_data, total_length, chunk_data_length, flags);
        }
    };

    class ToServerSender : public VirtualChannelDataSender
    {
        Transport&      transport;
        CryptContext&   encrypt;
        int             encryption_level;
        uint16_t        user_id;
        uint16_t        channel_id;
        bool            show_protocol;

        uint32_t verbose;

    public:
        ToServerSender(Transport& transport,
                       CryptContext& encrypt,
                       int encryption_level,
                       uint16_t user_id,
                       uint16_t channel_id,
                       bool show_protocol,
                       uint32_t verbose)
        : transport(transport)
        , encrypt(encrypt)
        , encryption_level(encryption_level)
        , user_id(user_id)
        , channel_id(channel_id)
        , show_protocol(show_protocol)
        , verbose(verbose) {}

        void operator()(uint32_t total_length, uint32_t flags,
            const uint8_t* chunk_data, uint32_t chunk_data_length)
                override {
            CHANNELS::VirtualChannelPDU virtual_channel_pdu;

            if (this->show_protocol) {
                flags |= CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL;
            }

            if ((this->verbose & MODRDP_LOGLEVEL_CLIPRDR_DUMP) ||
                (this->verbose & MODRDP_LOGLEVEL_RDPDR_DUMP)) {
                const bool send              = true;
                const bool from_or_to_client = false;
                ::msgdump_c(send, from_or_to_client, total_length, flags,
                    chunk_data, chunk_data_length);
            }

            virtual_channel_pdu.send_to_server(this->transport,
                this->encrypt, this->encryption_level, this->user_id,
                this->channel_id, total_length, flags, chunk_data,
                chunk_data_length);
        }
    };

    RDPChannelManagerMod(const uint16_t front_width,
        const uint16_t front_height, FrontAPI& front)
    : mod_api(front_width, front_height)
    , front(front) {}

    virtual std::unique_ptr<VirtualChannelDataSender> create_to_client_sender(
        const char* channel_name) const = 0;

    virtual std::unique_ptr<VirtualChannelDataSender> create_to_server_sender(
        const char* channel_name) = 0;

public:
    inline ClipboardVirtualChannel& get_clipboard_virtual_channel() {
        if (!this->clipboard_virtual_channel) {
            REDASSERT(!this->clipboard_to_client_sender &&
                !this->clipboard_to_server_sender);

            this->clipboard_to_client_sender =
                this->create_to_client_sender(channel_names::cliprdr);
            this->clipboard_to_server_sender =
                this->create_to_server_sender(channel_names::cliprdr);

            this->clipboard_virtual_channel =
                std::make_unique<ClipboardVirtualChannel>(
                    this->clipboard_to_client_sender.get(),
                    this->clipboard_to_server_sender.get(),
                    this->front,
                    this->get_clipboard_virtual_channel_params());
        }

        return *this->clipboard_virtual_channel;
    }

    inline FileSystemVirtualChannel& get_file_system_virtual_channel() {
        if (!this->file_system_virtual_channel) {
            REDASSERT(!this->file_system_to_client_sender &&
                !this->file_system_to_server_sender);

            this->file_system_to_client_sender =
                this->create_to_client_sender(channel_names::rdpdr);
            this->file_system_to_server_sender =
                this->create_to_server_sender(channel_names::rdpdr);

            this->file_system_virtual_channel =
                std::make_unique<FileSystemVirtualChannel>(
                    this->file_system_to_client_sender.get(),
                    this->file_system_to_server_sender.get(),
                    this->file_system_drive_manager,
                    this->front,
                    this->get_file_system_virtual_channel_params());
        }

        return *this->file_system_virtual_channel;
    }

protected:
    virtual const ClipboardVirtualChannel::Params
        get_clipboard_virtual_channel_params() const = 0;

    virtual const FileSystemVirtualChannel::Params
        get_file_system_virtual_channel_params() const = 0;
};  // RDPChannelManagerMod

class mod_rdp : public RDPChannelManagerMod {
    CHANNELS::ChannelDefArray mod_channel_list;

    const AuthorizationChannels authorization_channels;

    data_size_type max_clipboard_data = 0;
    data_size_type max_rdpdr_data = 0;

    int  use_rdp5;

    int  keylayout;

    uint8_t   lic_layer_license_key[16];
    uint8_t   lic_layer_license_sign_key[16];
    std::unique_ptr<uint8_t[]> lic_layer_license_data;
    size_t    lic_layer_license_size;

    rdp_orders orders;

    int      share_id;
    uint16_t userid;

    char hostname[16];
    char username[128];
    char password[2048];
    char domain[256];
    char program[512];
    char directory[512];

    char client_name[128];

    uint8_t bpp;

    int encryptionLevel;
    int encryptionMethod;

    const int    key_flags;

    uint32_t     server_public_key_len;
    uint8_t      client_crypt_random[512];
    CryptContext encrypt, decrypt;

    enum {
          MOD_RDP_NEGO
        , MOD_RDP_BASIC_SETTINGS_EXCHANGE
        , MOD_RDP_CHANNEL_CONNECTION_ATTACH_USER
        , MOD_RDP_GET_LICENSE
        , MOD_RDP_CONNECTED
    };

    enum {
        EARLY,
        WAITING_SYNCHRONIZE,
        WAITING_CTL_COOPERATE,
        WAITING_GRANT_CONTROL_COOPERATE,
        WAITING_FONT_MAP,
        UP_AND_RUNNING
    } connection_finalization_state;

    int state;
    Pointer cursors[32];
    const bool console_session;
    const uint8_t front_bpp;
    const uint32_t performanceFlags;
    Random & gen;
    const uint32_t verbose;
    const uint32_t cache_verbose;

    const bool enable_auth_channel;

    char auth_channel[8];
    int  auth_channel_flags;
    int  auth_channel_chanid;
    //int  auth_channel_state;    // 0 means unused, 1 means session running

    auth_api * acl;

    RdpNego nego;

    char clientAddr[512];

    const bool enable_fastpath;                    // choice of programmer
          bool enable_fastpath_client_input_event; // choice of programmer + capability of server
    const bool enable_fastpath_server_update;      // = choice of programmer
    const bool enable_glyph_cache;
    const bool enable_session_probe;
    const bool enable_session_probe_loading_mask;
    const bool enable_mem3blt;
    const bool enable_new_pointer;
    const bool enable_transparent_mode;
    const bool enable_persistent_disk_bitmap_cache;
    const bool enable_cache_waiting_list;
    const bool persist_bitmap_cache_on_disk;
    const bool disable_clipboard_log_syslog;
    const bool disable_clipboard_log_wrm;
    const bool disable_file_system_log_syslog;
    const bool disable_file_system_log_wrm;
    const int  rdp_compression;

    const unsigned    session_probe_launch_timeout;
    const bool        session_probe_on_launch_failure_disconnect_user;
    const unsigned    session_probe_keepalive_timeout;
          std::string session_probe_alternate_shell;

    size_t recv_bmp_update;

    rdp_mppc_unified_dec mppc_dec;

    std::string * error_message;

    const bool     disconnect_on_logon_user_change;
    const uint32_t open_session_timeout;

    TimeoutT<time_t> open_session_timeout_checker;

    std::string output_filename;

    std::string end_session_reason;
    std::string end_session_message;

    const bool                     server_cert_store;
    const configs::ServerCertCheck server_cert_check;

    const char * certif_path;

    bool enable_polygonsc;
    bool enable_polygoncb;
    bool enable_polyline;
    bool enable_ellipsesc;
    bool enable_ellipsecb;
    bool enable_multidstblt;
    bool enable_multiopaquerect;
    bool enable_multipatblt;
    bool enable_multiscrblt;

    const bool remote_program;

    const bool server_redirection_support;

    TransparentRecorder * transparent_recorder;
    Transport           * persistent_key_list_transport;

    //uint64_t total_data_received;

    const uint32_t password_printing_mode;

    bool deactivation_reactivation_in_progress;

    RedirectionInfo & redir_info;

    const bool bogus_sc_net_size;

    std::string real_alternate_shell;
    std::string real_working_dir;

    wait_obj    session_probe_event;
    bool        session_probe_is_ready            = false;
    bool        session_probe_keep_alive_received = true;

    std::deque<std::unique_ptr<AsynchronousTask>> asynchronous_tasks;
    wait_obj                                      asynchronous_task_event;

    Translation::language_t lang;

    OutboundConnectionMonitorRules outbound_connection_monitor_rules;

    class ToServerAsynchronousSender : public VirtualChannelDataSender
    {
        std::unique_ptr<VirtualChannelDataSender> to_server_synchronous_sender;

        std::deque<std::unique_ptr<AsynchronousTask>> & asynchronous_tasks;

        wait_obj & asynchronous_task_event;

        uint32_t verbose;

    public:
        ToServerAsynchronousSender(
            std::unique_ptr<VirtualChannelDataSender> &
                to_server_synchronous_sender,
            std::deque<std::unique_ptr<AsynchronousTask>> &
                asynchronous_tasks,
            wait_obj & asynchronous_task_event,
            uint32_t verbose)
        : to_server_synchronous_sender(
            std::move(to_server_synchronous_sender))
        , asynchronous_tasks(asynchronous_tasks)
        , asynchronous_task_event(asynchronous_task_event)
        , verbose(verbose) {}

        VirtualChannelDataSender& SynchronousSender() override {
            return *(to_server_synchronous_sender.get());
        }

        void operator()(uint32_t total_length, uint32_t flags,
            const uint8_t* chunk_data, uint32_t chunk_data_length)
                override {
            std::unique_ptr<AsynchronousTask> asynchronous_task =
                std::make_unique<RdpdrSendClientMessageTask>(
                    total_length, flags, chunk_data, chunk_data_length,
                    *(this->to_server_synchronous_sender.get()),
                    this->verbose);

            if (this->asynchronous_tasks.empty()) {
                this->asynchronous_task_event.~wait_obj();
                new (&this->asynchronous_task_event) wait_obj();

                asynchronous_task->configure_wait_object(
                    this->asynchronous_task_event);
            }

            this->asynchronous_tasks.push_back(std::move(asynchronous_task));
        }
    };

    TODO("duplicated code in front")
    struct write_x224_dt_tpdu_fn
    {
        void operator()(StreamSize<7>, OutStream & x224_header, std::size_t sz) const {
            X224::DT_TPDU_Send(x224_header, sz);
        }
    };

    struct write_sec_send_fn
    {
        uint32_t flags;
        CryptContext & encrypt;
        int encryption_level;

        void operator()(StreamSize<256>, OutStream & sec_header, uint8_t * packet_data, std::size_t packet_size) const {
            SEC::Sec_Send sec(sec_header, packet_data, packet_size, this->flags, this->encrypt, this->encryption_level);
            (void)sec;
        }
    };

    class RDPServerNotifier : public ServerNotifier {
    private:
        auth_api * acl;

        const configs::ServerNotification server_access_allowed_message;
        const configs::ServerNotification server_cert_create_message;
        const configs::ServerNotification server_cert_success_message;
        const configs::ServerNotification server_cert_failure_message;
        const configs::ServerNotification server_cert_error_message;

        uint32_t verbose;

        bool is_syslog_notification_enabled(
                configs::ServerNotification server_notification) {
            return
                ((server_notification & configs::ServerNotification::syslog) ==
                 configs::ServerNotification::syslog);
        }

    public:
        RDPServerNotifier(
                auth_api * acl,
                configs::ServerNotification server_access_allowed_message,
                configs::ServerNotification server_cert_create_message,
                configs::ServerNotification server_cert_success_message,
                configs::ServerNotification server_cert_failure_message,
                configs::ServerNotification server_cert_error_message,
                uint32_t verbose
            )
        : acl(acl)
        , server_access_allowed_message(server_access_allowed_message)
        , server_cert_create_message(server_cert_create_message)
        , server_cert_success_message(server_cert_success_message)
        , server_cert_failure_message(server_cert_failure_message)
        , server_cert_error_message(server_cert_error_message)
        , verbose(verbose)
        {}

        void server_access_allowed() override {
            if (is_syslog_notification_enabled(
                    this->server_access_allowed_message) &&
                this->acl) {
                this->acl->log4((this->verbose & 1),
                        "certificate_check_success",
                        "data=\"Connexion to server allowed\""
                    );
            }
        }

        void server_cert_create() override {
            if (is_syslog_notification_enabled(
                    this->server_cert_create_message) &&
                this->acl) {
                this->acl->log4((this->verbose & 1),
                        "server_certificate_new",
                        "data=\"New X.509 certificate created\""
                    );
            }
        }

        void server_cert_success() override {
            if (is_syslog_notification_enabled(
                    this->server_cert_success_message) &&
                this->acl) {
                this->acl->log4((this->verbose & 1),
                        "server_certificate_match_success",
                        "data=\"X.509 server certificate match\""
                    );
            }
        }

        void server_cert_failure() override {
            if (is_syslog_notification_enabled(
                    this->server_cert_failure_message) &&
                this->acl) {
                this->acl->log4((this->verbose & 1),
                        "server_certificate_match_failure",
                        "data=\"X.509 server certificate match failure\""
                    );
            }
        }

        void server_cert_error(const char * str_error) override {
            if (is_syslog_notification_enabled(
                    this->server_cert_error_message) &&
                this->acl) {
                char extra[512];
                snprintf(extra, sizeof(extra),
                        "data=\"X.509 server certificate internal error: '%s'\"",
                        (str_error ? str_error : "")
                    );
                this->acl->log4((this->verbose & 1),
                        "server_certificate_error",
                        extra
                    );
            }
        }
    } server_notifier;

    bool session_probe_close_pending = false;

public:
    mod_rdp( Transport & trans
           , FrontAPI & front
           , const ClientInfo & info
           , RedirectionInfo & redir_info
           , Random & gen
           , const ModRDPParams & mod_rdp_params
           )
        : RDPChannelManagerMod(info.width - (info.width % 4), info.height, front)
        , authorization_channels(
            mod_rdp_params.allow_channels ? *mod_rdp_params.allow_channels : std::string{},
            mod_rdp_params.deny_channels ? *mod_rdp_params.deny_channels : std::string{}
          )
        , use_rdp5(1)
        , keylayout(info.keylayout)
        , orders( mod_rdp_params.target_host, mod_rdp_params.enable_persistent_disk_bitmap_cache
                , mod_rdp_params.persist_bitmap_cache_on_disk, mod_rdp_params.verbose)
        , share_id(0)
        , userid(0)
        , bpp(0)
        , encryptionLevel(0)
        , key_flags(mod_rdp_params.key_flags)
        , server_public_key_len(0)
        , connection_finalization_state(EARLY)
        , state(MOD_RDP_NEGO)
        , console_session(info.console_session)
        , front_bpp(info.bpp)
        , performanceFlags(info.rdp5_performanceflags)
        , gen(gen)
        , verbose(mod_rdp_params.verbose)
        , cache_verbose(mod_rdp_params.cache_verbose)
        , enable_auth_channel(mod_rdp_params.alternate_shell[0] && !mod_rdp_params.ignore_auth_channel)
        , auth_channel_flags(0)
        , auth_channel_chanid(0)
        //, auth_channel_state(0) // 0 means unused
        , acl(mod_rdp_params.acl)
        , nego( mod_rdp_params.enable_tls, trans, mod_rdp_params.target_user
              , mod_rdp_params.enable_nla, mod_rdp_params.target_host
              , mod_rdp_params.enable_krb, gen, mod_rdp_params.verbose)
        , enable_fastpath(mod_rdp_params.enable_fastpath)
        , enable_fastpath_client_input_event(false)
        , enable_fastpath_server_update(mod_rdp_params.enable_fastpath)
        , enable_glyph_cache(mod_rdp_params.enable_glyph_cache)
        , enable_session_probe(mod_rdp_params.enable_session_probe)
        , enable_session_probe_loading_mask(mod_rdp_params.enable_session_probe_loading_mask)
        , enable_mem3blt(mod_rdp_params.enable_mem3blt)
        , enable_new_pointer(mod_rdp_params.enable_new_pointer)
        , enable_transparent_mode(mod_rdp_params.enable_transparent_mode)
        , enable_persistent_disk_bitmap_cache(mod_rdp_params.enable_persistent_disk_bitmap_cache)
        , enable_cache_waiting_list(mod_rdp_params.enable_cache_waiting_list)
        , persist_bitmap_cache_on_disk(mod_rdp_params.persist_bitmap_cache_on_disk)
        , disable_clipboard_log_syslog(mod_rdp_params.disable_clipboard_log_syslog)
        , disable_clipboard_log_wrm(mod_rdp_params.disable_clipboard_log_wrm)
        , disable_file_system_log_syslog(mod_rdp_params.disable_file_system_log_syslog)
        , disable_file_system_log_wrm(mod_rdp_params.disable_file_system_log_wrm)
        , rdp_compression(mod_rdp_params.rdp_compression)
        , session_probe_launch_timeout(mod_rdp_params.session_probe_launch_timeout)
        , session_probe_on_launch_failure_disconnect_user(mod_rdp_params.session_probe_on_launch_failure_disconnect_user)
        , session_probe_keepalive_timeout(mod_rdp_params.session_probe_keepalive_timeout)
        , session_probe_alternate_shell(mod_rdp_params.session_probe_alternate_shell)
        , recv_bmp_update(0)
        , error_message(mod_rdp_params.error_message)
        , disconnect_on_logon_user_change(mod_rdp_params.disconnect_on_logon_user_change)
        , open_session_timeout(mod_rdp_params.open_session_timeout)
        , open_session_timeout_checker(0)
        , output_filename(mod_rdp_params.output_filename)
        , server_cert_store(mod_rdp_params.server_cert_store)
        , server_cert_check(mod_rdp_params.server_cert_check)
        , certif_path([](const char * device_id){
            size_t lg_certif_path = strlen(CERTIF_PATH);
            size_t lg_dev_id = strlen(device_id);
            char * buffer(new(std::nothrow) char[lg_certif_path + lg_dev_id + 2]);
            if (!buffer){
                throw Error(ERR_PATH_TOO_LONG);
            }
            memcpy(buffer, CERTIF_PATH, lg_certif_path);
            buffer[lg_certif_path] =  '/';
            memcpy(buffer+lg_certif_path+1, device_id, lg_dev_id+1);
            return buffer;
        }(mod_rdp_params.device_id))

        , enable_polygonsc(false)
        , enable_polygoncb(false)
        , enable_polyline(false)
        , enable_ellipsesc(false)
        , enable_ellipsecb(false)
        , enable_multidstblt(false)
        , enable_multiopaquerect(false)
        , enable_multipatblt(false)
        , enable_multiscrblt(false)
        , remote_program(mod_rdp_params.remote_program)
        , server_redirection_support(mod_rdp_params.server_redirection_support)
        , transparent_recorder(nullptr)
        , persistent_key_list_transport(mod_rdp_params.persistent_key_list_transport)
        //, total_data_received(0)
        , password_printing_mode(mod_rdp_params.password_printing_mode)
        , deactivation_reactivation_in_progress(false)
        , redir_info(redir_info)
        , bogus_sc_net_size(mod_rdp_params.bogus_sc_net_size)
        , lang(mod_rdp_params.lang)
        , outbound_connection_monitor_rules("", mod_rdp_params.outbound_connection_blocking_rules)
        , server_notifier(mod_rdp_params.acl,
                          mod_rdp_params.server_access_allowed_message,
                          mod_rdp_params.server_cert_create_message,
                          mod_rdp_params.server_cert_success_message,
                          mod_rdp_params.server_cert_failure_message,
                          mod_rdp_params.server_cert_error_message,
                          mod_rdp_params.verbose
                         )
    {
        if (this->verbose & 1) {
            if (!enable_transparent_mode) {
                LOG(LOG_INFO, "Creation of new mod 'RDP'");
                LOG(LOG_INFO, "Creation of new mod 'RDP'");
            }
            else {
                LOG(LOG_INFO, "Creation of new mod 'RDP Transparent'");

                if (this->output_filename.empty()) {
                    LOG(LOG_INFO, "Use transparent capabilities.");
                }
                else {
                    LOG(LOG_INFO, "Use proxy default capabilities.");
                }
            }

            mod_rdp_params.log();
        }

        if (this->enable_session_probe) {
            this->file_system_drive_manager.EnableSessionProbeDrive(this->verbose);
        }

        if (mod_rdp_params.proxy_managed_drives && (*mod_rdp_params.proxy_managed_drives)) {
            this->configure_proxy_managed_drives(mod_rdp_params.proxy_managed_drives);
        }

        if (mod_rdp_params.transparent_recorder_transport) {
            this->transparent_recorder = new TransparentRecorder(mod_rdp_params.transparent_recorder_transport);
        }

        this->configure_extra_orders(mod_rdp_params.extra_orders);

        this->event.object_and_time = (this->open_session_timeout > 0);

        memset(this->auth_channel, 0, sizeof(this->auth_channel));
        strncpy(this->auth_channel,
                ((!(*mod_rdp_params.auth_channel) ||
                  !strncmp(mod_rdp_params.auth_channel, "*", 2)) ? "wablnch"
                                                               : mod_rdp_params.auth_channel),
                sizeof(this->auth_channel) - 1);

        memset(this->clientAddr, 0, sizeof(this->clientAddr));
        strncpy(this->clientAddr, mod_rdp_params.client_address, sizeof(this->clientAddr) - 1);
        this->lic_layer_license_size = 0;
        memset(this->lic_layer_license_key, 0, 16);
        memset(this->lic_layer_license_sign_key, 0, 16);
        TODO("CGR: license loading should be done before creating protocol layers");
        struct stat st;
        char path[256];
        snprintf(path, sizeof(path), LICENSE_PATH "/license.%s", info.hostname);
        int fd = open(path, O_RDONLY);
        if (fd != -1){
            if (fstat(fd, &st) != 0){
                this->lic_layer_license_data.reset(new uint8_t[this->lic_layer_license_size]);
                if (this->lic_layer_license_data){
                    size_t lic_size = read(fd, this->lic_layer_license_data.get(), this->lic_layer_license_size);
                    if (lic_size != this->lic_layer_license_size){
                        LOG(LOG_ERR, "license file truncated : expected %zu, got %zu", this->lic_layer_license_size, lic_size);
                    }
                }
            }
            close(fd);
        }

        // from rdp_sec
        memset(this->client_crypt_random, 0, sizeof(this->client_crypt_random));

        // shared
        memset(this->decrypt.key, 0, 16);
        memset(this->encrypt.key, 0, 16);
        memset(this->decrypt.update_key, 0, 16);
        memset(this->encrypt.update_key, 0, 16);
        this->decrypt.encryptionMethod = 2; /* 128 bits */
        this->encrypt.encryptionMethod = 2; /* 128 bits */

        if (::strlen(info.hostname) >= sizeof(this->hostname)) {
            LOG(LOG_WARNING, "mod_rdp: hostname too long! %zu >= %zu", ::strlen(info.hostname), sizeof(this->hostname));
        }
        strncpy(this->hostname, info.hostname, 15);
        this->hostname[15] = 0;


        const char * domain_pos   = nullptr;
        size_t       domain_len   = 0;
        const char * username_pos = nullptr;
        size_t       username_len = 0;
        const char * separator = strchr(mod_rdp_params.target_user, '\\');
        if (separator)
        {
            domain_pos   = mod_rdp_params.target_user;
            domain_len   = separator - mod_rdp_params.target_user;
            username_pos = ++separator;
            username_len = strlen(username_pos);
        }
        else
        {
            separator = strchr(mod_rdp_params.target_user, '@');
            if (separator)
            {
                domain_pos   = separator + 1;
                domain_len   = strlen(domain_pos);
                username_pos = mod_rdp_params.target_user;
                username_len = separator - mod_rdp_params.target_user;
                LOG(LOG_INFO, "mod_rdp: username_len=%zu", username_len);
            }
            else
            {
                username_pos = mod_rdp_params.target_user;
                username_len = strlen(username_pos);
            }
        }

        if (username_len >= sizeof(this->username)) {
            LOG(LOG_INFO, "mod_rdp: username too long! %zu >= %zu", username_len, sizeof(this->username));
        }
        size_t count = std::min(sizeof(this->username) - 1, username_len);
        if (count > 0) strncpy(this->username, username_pos, count);
        this->username[count] = 0;

        if (domain_len >= sizeof(this->domain)) {
            LOG(LOG_INFO, "mod_rdp: domain too long! %zu >= %zu", domain_len, sizeof(this->domain));
        }
        count = std::min(sizeof(this->domain) - 1, domain_len);
        if (count > 0) strncpy(this->domain, domain_pos, count);
        this->domain[count] = 0;

        LOG(LOG_INFO, "Remote RDP Server domain=\"%s\" login=\"%s\" host=\"%s\"",
            this->domain, this->username, this->hostname);


        // Password is a multi-sz!
        // A multi-sz contains a sequence of null-terminated strings,
        //  terminated by an empty string (\0) so that the last two
        //  characters are both null terminators.
        SOHSeparatedStringsToMultiSZ(this->password, sizeof(this->password), mod_rdp_params.target_password);

        snprintf(this->client_name, sizeof(this->client_name), "%s", mod_rdp_params.client_name);

        std::string alternate_shell(mod_rdp_params.alternate_shell);
        if (mod_rdp_params.target_application_account && *mod_rdp_params.target_application_account) {
            const char * user_marker = "${USER}";
            size_t pos = alternate_shell.find(user_marker, 0);
            if (pos != std::string::npos) {
                alternate_shell.replace(pos, strlen(user_marker), mod_rdp_params.target_application_account);
            }
        }
        if (mod_rdp_params.target_application_password && *mod_rdp_params.target_application_password) {
            const char * password_marker = "${PASSWORD}";
            size_t pos = alternate_shell.find(password_marker, 0);
            if (pos != std::string::npos) {
                alternate_shell.replace(pos, strlen(password_marker), mod_rdp_params.target_application_password);
            }
        }

        if (this->enable_session_probe) {
            this->real_alternate_shell = std::move(alternate_shell);
            this->real_working_dir     = mod_rdp_params.shell_working_directory;

            char pid_str[16];
            snprintf(pid_str, sizeof(pid_str), "%d", ::getpid());
            const size_t pid_str_len = ::strlen(pid_str);

            const char * pid_tag ="{PID}";
            const size_t pid_tag_len = ::strlen(pid_tag);

            size_t pos = 0;
            while ((pos = this->session_probe_alternate_shell.find(pid_tag, pos)) != std::string::npos) {
                this->session_probe_alternate_shell.replace(pos, pid_tag_len, pid_str);
                pos += pid_str_len;
            }

            strncpy(this->program, this->session_probe_alternate_shell.c_str(), sizeof(this->program) - 1);
            this->program[sizeof(this->program) - 1] = 0;

            const char * session_probe_working_dir = "%TMP%";
            strncpy(this->directory, session_probe_working_dir, sizeof(this->directory) - 1);
            this->directory[sizeof(this->directory) - 1] = 0;
        }
        else {
            strncpy(this->program, alternate_shell.c_str(), sizeof(this->program) - 1);
            this->program[sizeof(this->program) - 1] = 0;
            strncpy(this->directory, mod_rdp_params.shell_working_directory, sizeof(this->directory) - 1);
            this->directory[sizeof(this->directory) - 1] = 0;
        }

        LOG(LOG_INFO, "Server key layout is %x", this->keylayout);

        this->nego.set_identity(this->username,
                                this->domain,
                                this->password,
                                this->hostname);

        if (this->verbose & 128){
            this->redir_info.log(LOG_INFO, "Init with Redir_info");
            LOG(LOG_INFO, "ServerRedirectionSupport=%s",
                this->server_redirection_support ? "true" : "false");
        }
        if (this->server_redirection_support) {
            if (this->redir_info.valid && (this->redir_info.lb_info_length > 0)) {
                this->nego.set_lb_info(this->redir_info.lb_info,
                                       this->redir_info.lb_info_length);
            }
        }

        while (UP_AND_RUNNING != this->connection_finalization_state){
            this->draw_event(time(nullptr));
            if (this->event.signal != BACK_EVENT_NONE){
                char statestr[256];
                switch (this->state) {
                case MOD_RDP_NEGO:
                    snprintf(statestr, sizeof(statestr), "RDP_NEGO");
                    break;
                case MOD_RDP_BASIC_SETTINGS_EXCHANGE:
                    snprintf(statestr, sizeof(statestr), "RDP_BASIC_SETTINGS_EXCHANGE");
                    break;
                case MOD_RDP_CHANNEL_CONNECTION_ATTACH_USER:
                    snprintf(statestr, sizeof(statestr),
                             "RDP_CHANNEL_CONNECTION_ATTACH_USER");
                    break;
                case MOD_RDP_GET_LICENSE:
                    snprintf(statestr, sizeof(statestr), "RDP_GET_LICENSE");
                    break;
                case MOD_RDP_CONNECTED:
                    snprintf(statestr, sizeof(statestr), "RDP_CONNECTED");
                    break;
                default:
                    snprintf(statestr, sizeof(statestr), "UNKNOWN");
                    break;
                }
                statestr[255] = 0;
                LOG(LOG_ERR, "Creation of new mod 'RDP' failed at %s state", statestr);
                throw Error(ERR_SESSION_UNKNOWN_BACKEND);
            }
        }

        if (this->verbose & 1) {
            LOG(LOG_INFO,
                "enable_session_probe=%s session_probe_launch_timeout=%u session_probe_on_launch_failure_disconnect_user=%s",
                (this->enable_session_probe ? "yes" : "no"), this->session_probe_launch_timeout,
                (this->session_probe_on_launch_failure_disconnect_user ? "yes" : "no"));
        }
        if (this->enable_session_probe) {
            this->session_probe_event.object_and_time = true;

            if (this->session_probe_launch_timeout > 0) {
                if (this->verbose & 1) {
                    LOG(LOG_INFO, "Enable Session Probe launch timer");
                }
                this->session_probe_event.set(this->session_probe_launch_timeout * 1000);
            }
        }

        if (this->acl) {
            this->acl->report("CONNECTION_SUCCESSFUL", "OK.");
        }

        // this->end_session_reason.copy_c_str("OPEN_SESSION_FAILED");
        // this->end_session_message.copy_c_str("Open RDP session cancelled.");
    }   // mod_rdp

    ~mod_rdp() override {
        if (this->enable_session_probe && this->enable_session_probe_loading_mask) {
            this->front.disable_input_event_and_graphics_update(false);
        }

        delete this->transparent_recorder;

        if (this->acl && !this->end_session_reason.empty() &&
            !this->end_session_message.empty()) {
            this->acl->report(this->end_session_reason.c_str(),
                this->end_session_message.c_str());
        }

        if (this->verbose & 1) {
            LOG(LOG_INFO, "~mod_rdp(): Recv bmp cache count  = %zu",
                this->orders.recv_bmp_cache_count);
            LOG(LOG_INFO, "~mod_rdp(): Recv order count      = %zu",
                this->orders.recv_order_count);
            LOG(LOG_INFO, "~mod_rdp(): Recv bmp update count = %zu",
                this->recv_bmp_update);
        }
        delete [] this->certif_path;
    }

protected:
    std::unique_ptr<VirtualChannelDataSender> create_to_client_sender(
            const char* channel_name) const override
    {
        if (!this->authorization_channels.is_authorized(channel_name))
        {
            return nullptr;
        }

        const CHANNELS::ChannelDefArray& front_channel_list =
            this->front.get_channel_list();

        const CHANNELS::ChannelDef* channel =
            front_channel_list.get_by_name(channel_name);
        if (!channel)
        {
            return nullptr;
        }

        std::unique_ptr<ToClientSender> to_client_sender =
            std::make_unique<ToClientSender>(this->front, *channel,
                this->verbose);

        return std::unique_ptr<VirtualChannelDataSender>(
            std::move(to_client_sender));
    }

    std::unique_ptr<VirtualChannelDataSender> create_to_server_sender(
            const char* channel_name) override
    {
        const CHANNELS::ChannelDef* channel =
            this->mod_channel_list.get_by_name(channel_name);
        if (!channel)
        {
            return nullptr;
        }

        std::unique_ptr<ToServerSender> to_server_sender =
            std::make_unique<ToServerSender>(
                this->nego.trans,
                this->encrypt,
                this->encryptionLevel,
                this->userid,
                channel->chanid,
                (channel->flags &
                 GCC::UserData::CSNet::CHANNEL_OPTION_SHOW_PROTOCOL),
                this->verbose);

        if (strcmp(channel_name, channel_names::rdpdr)) {
            return std::unique_ptr<VirtualChannelDataSender>(
                std::move(to_server_sender));
        }

        std::unique_ptr<VirtualChannelDataSender>
            virtual_channel_data_sender(std::move(to_server_sender));

        std::unique_ptr<ToServerAsynchronousSender>
            to_server_asynchronous_sender =
                std::make_unique<ToServerAsynchronousSender>(
                    virtual_channel_data_sender,
                    this->asynchronous_tasks,
                    this->asynchronous_task_event,
                    this->verbose);

        return std::unique_ptr<VirtualChannelDataSender>(
            std::move(to_server_asynchronous_sender));
    }

    const ClipboardVirtualChannel::Params
        get_clipboard_virtual_channel_params() const override
    {
        ClipboardVirtualChannel::Params clipboard_virtual_channel_params;

        clipboard_virtual_channel_params.authentifier                    =
            this->acl;
        clipboard_virtual_channel_params.exchanged_data_limit            =
            this->max_clipboard_data;
        clipboard_virtual_channel_params.verbose                         =
            this->verbose;

        clipboard_virtual_channel_params.clipboard_down_authorized       =
            this->authorization_channels.cliprdr_down_is_authorized();
        clipboard_virtual_channel_params.clipboard_up_authorized         =
            this->authorization_channels.cliprdr_up_is_authorized();
        clipboard_virtual_channel_params.clipboard_file_authorized       =
            this->authorization_channels.cliprdr_file_is_authorized();

        clipboard_virtual_channel_params.dont_log_data_into_syslog       =
            this->disable_clipboard_log_syslog;
        clipboard_virtual_channel_params.dont_log_data_into_wrm          =
            this->disable_clipboard_log_wrm;

        clipboard_virtual_channel_params.acl                             =
            this->acl;

        return clipboard_virtual_channel_params;
    }

    const FileSystemVirtualChannel::Params
        get_file_system_virtual_channel_params() const override
    {
        FileSystemVirtualChannel::Params file_system_virtual_channel_params;

        file_system_virtual_channel_params.authentifier                    =
            this->acl;
        file_system_virtual_channel_params.exchanged_data_limit            =
            this->max_rdpdr_data;
        file_system_virtual_channel_params.verbose                         =
            this->verbose;

        file_system_virtual_channel_params.client_name                     =
            this->client_name;
        file_system_virtual_channel_params.file_system_read_authorized     =
            this->authorization_channels.rdpdr_drive_read_is_authorized();
        file_system_virtual_channel_params.file_system_write_authorized    =
            this->authorization_channels.rdpdr_drive_write_is_authorized();
        file_system_virtual_channel_params.parallel_port_authorized        =
            this->authorization_channels.rdpdr_type_is_authorized(
                rdpdr::RDPDR_DTYP_PARALLEL);
        file_system_virtual_channel_params.print_authorized                =
            this->authorization_channels.rdpdr_type_is_authorized(
                rdpdr::RDPDR_DTYP_PRINT);
        file_system_virtual_channel_params.serial_port_authorized          =
            this->authorization_channels.rdpdr_type_is_authorized(
                rdpdr::RDPDR_DTYP_SERIAL);
        file_system_virtual_channel_params.smart_card_authorized           =
            this->authorization_channels.rdpdr_type_is_authorized(
                rdpdr::RDPDR_DTYP_SMARTCARD);
        file_system_virtual_channel_params.random_number                   =
            ::getpid();

        file_system_virtual_channel_params.dont_log_data_into_syslog       =
            this->disable_file_system_log_syslog;
        file_system_virtual_channel_params.dont_log_data_into_wrm          =
            this->disable_file_system_log_wrm;

        file_system_virtual_channel_params.acl                             =
            this->acl;

        return file_system_virtual_channel_params;
    }

public:
    void configure_extra_orders(const char * extra_orders) {
        if (verbose) {
            LOG(LOG_INFO, "RDP Extra orders=\"%s\"", extra_orders);
        }

        apply_for_delim(extra_orders, ',', [this](const char * order) {
            int const order_number = long_from_cstr(order);
            if (verbose) {
                LOG(LOG_INFO, "RDP Extra orders number=%d", order_number);
            }
            switch (order_number) {
            case RDP::MULTIDSTBLT:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=MultiDstBlt");
                }
                this->enable_multidstblt = true;
                break;
            case RDP::MULTIOPAQUERECT:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=MultiOpaqueRect");
                }
                this->enable_multiopaquerect = true;
                break;
            case RDP::MULTIPATBLT:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=MultiPatBlt");
                }
                this->enable_multipatblt = true;
                break;
            case RDP::MULTISCRBLT:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=MultiScrBlt");
                }
                this->enable_multiscrblt = true;
                break;
            case RDP::POLYGONSC:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=PolygonSC");
                }
                this->enable_polygonsc = true;
                break;
            case RDP::POLYGONCB:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=PolygonCB");
                }
                this->enable_polygoncb = true;
                break;
            case RDP::POLYLINE:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=Polyline");
                }
                this->enable_polyline = true;
                break;
            case RDP::ELLIPSESC:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=EllipseSC");
                }
                this->enable_ellipsesc = true;
                break;
            case RDP::ELLIPSECB:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Extra orders=EllipseCB");
                }
                this->enable_ellipsecb = true;
                break;
            default:
                if (verbose) {
                    LOG(LOG_INFO, "RDP Unknown Extra orders");
                }
                break;
            }
        }, [](char c) { return c == ' ' || c == '\t' || c == ','; });
    }   // configure_extra_orders

    void configure_proxy_managed_drives(const char * proxy_managed_drives) {
        if (verbose) {
            LOG(LOG_INFO, "Proxy managed drives=\"%s\"", proxy_managed_drives);
        }

        const bool complete_item_extraction = true;
        apply_for_delim(proxy_managed_drives,
                        ',',
                        [this](const char * drive) {
                                if (verbose) {
                                    LOG(LOG_INFO, "Proxy managed drive=\"%s\"", drive);
                                }
                                this->file_system_drive_manager.EnableDrive(drive, this->verbose);
                            },
                        is_blanck_fn(),
                        complete_item_extraction
                       );
    }   // configure_proxy_managed_drives

    void rdp_input_scancode( long param1, long param2, long device_flags, long time
                                     , Keymap2 * keymap) override {
        if (UP_AND_RUNNING == this->connection_finalization_state) {
            this->send_input(time, RDP_INPUT_SCANCODE, device_flags, param1, param2);
        }
    }

    void rdp_input_synchronize( uint32_t time, uint16_t device_flags, int16_t param1
                                        , int16_t param2) override {
        if (UP_AND_RUNNING == this->connection_finalization_state) {
            this->send_input(0, RDP_INPUT_SYNCHRONIZE, device_flags, param1, 0);
        }
    }

    void rdp_input_mouse(int device_flags, int x, int y, Keymap2 * keymap) override {
        if (UP_AND_RUNNING == this->connection_finalization_state) {
            this->send_input(0, RDP_INPUT_MOUSE, device_flags, x, y);
        }
    }

    void send_to_front_channel( const char * const mod_channel_name, uint8_t const * data
                              , size_t length, size_t chunk_size, int flags) override {
        if (this->transparent_recorder) {
            this->transparent_recorder->send_to_front_channel( mod_channel_name, data, length
                                                             , chunk_size, flags);
        }

        const CHANNELS::ChannelDef * front_channel = this->front.get_channel_list().get_by_name(mod_channel_name);
        if (front_channel) {
            this->front.send_to_channel(*front_channel, data, length, chunk_size, flags);
        }
    }

public:
    wait_obj * get_asynchronous_task_event(int & out_fd) override {
        if (this->asynchronous_tasks.empty()) {
            out_fd = -1;
            return nullptr;
        }

        out_fd = this->asynchronous_tasks.front()->get_file_descriptor();

        return &this->asynchronous_task_event;
    }

    void process_asynchronous_task() override {
        if (!this->asynchronous_tasks.front()->run(this->asynchronous_task_event)) {
            this->asynchronous_tasks.pop_front();
        }

        this->asynchronous_task_event.~wait_obj();
        new (&this->asynchronous_task_event) wait_obj();

        if (!this->asynchronous_tasks.empty()) {
            this->asynchronous_tasks.front()->configure_wait_object(this->asynchronous_task_event);
        }
    }

    void send_to_mod_channel( const char * const front_channel_name
                                    , InStream & chunk
                                    , size_t length
                                    , uint32_t flags) override {
        if (this->verbose & 16) {
            LOG(LOG_INFO,
                "mod_rdp::send_to_mod_channel: front_channel_channel=\"%s\"",
                front_channel_name);
        }

        const CHANNELS::ChannelDef * mod_channel = this->mod_channel_list.get_by_name(front_channel_name);
        if (!mod_channel) {
            return;
        }
        if (this->verbose & 16) {
            mod_channel->log(unsigned(mod_channel - &this->mod_channel_list[0]));
        }

             if (!strcmp(front_channel_name, channel_names::cliprdr)) {
            this->send_to_mod_cliprdr_channel(mod_channel, chunk, length, flags);
        }
        else if (!strcmp(front_channel_name, channel_names::rail)) {
            this->send_to_mod_rail_channel(mod_channel, chunk, length, flags);
        }
        else if (!strcmp(front_channel_name, channel_names::rdpdr)) {
            this->send_to_mod_rdpdr_channel(mod_channel, chunk, length, flags);
        }
        else {
            this->send_to_channel(*mod_channel, chunk.get_data(), chunk.get_capacity(), length, flags);
        }
    }

private:
    void send_to_mod_cliprdr_channel(const CHANNELS::ChannelDef * cliprdr_channel,
                                     InStream & chunk, size_t length, uint32_t flags) {
        BaseVirtualChannel& channel = this->get_clipboard_virtual_channel();

        channel.process_client_message(length, flags, chunk.get_current(), chunk.in_remain());
    }

    void send_to_mod_rail_channel(const CHANNELS::ChannelDef * rail_channel,
                                  InStream & chunk, size_t length, uint32_t flags) {
        //LOG(LOG_INFO, "mod_rdp::send_to_mod_rail_channel: chunk.size=%u length=%u",
        //    chunk.size(), length);
        //hexdump_d(chunk.get_data(), chunk.size());

        const auto saved_chunk_p = chunk.get_current();

        const uint16_t orderType   = chunk.in_uint16_le();
        const uint16_t orderLength = chunk.in_uint16_le();

        //LOG(LOG_INFO, "mod_rdp::send_to_mod_rail_channel: orderType=%u orderLength=%u",
        //    orderType, orderLength);

        switch (orderType) {
            case TS_RAIL_ORDER_EXEC:
            {
                ClientExecutePDU_Recv cepdur(chunk);

                LOG(LOG_INFO,
                    "mod_rdp::send_to_mod_rail_channel: Client Execute PDU - "
                        "flags=0x%X exe_or_file=\"%s\" working_dir=\"%s\" arguments=\"%s\"",
                    cepdur.Flags(), cepdur.exe_or_file(), cepdur.working_dir(), cepdur.arguments());
            }
            break;

            case TS_RAIL_ORDER_SYSPARAM:
            {
                ClientSystemParametersUpdatePDU_Recv cspupdur(chunk);

                switch(cspupdur.SystemParam()) {
                    case SPI_SETDRAGFULLWINDOWS:
                    {
                        const unsigned expected = 1 /* Body(1) */;
                        if (!chunk.in_check_rem(expected)) {
                            LOG(LOG_ERR,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "expected=%u remains=%zu (0x%04X)",
                                expected, chunk.in_remain(),
                                cspupdur.SystemParam());
                            throw Error(ERR_RAIL_PDU_TRUNCATED);
                        }

                        uint8_t Body = chunk.in_uint8();

                        LOG(LOG_INFO,
                            "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                "Full Window Drag is %s.",
                            (!Body ? "disabled" : "enabled"));
                    }
                    break;

                    case SPI_SETKEYBOARDCUES:
                    {
                        const unsigned expected = 1 /* Body(1) */;
                        if (!chunk.in_check_rem(expected)) {
                            LOG(LOG_ERR,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "expected=%u remains=%zu (0x%04X)",
                                expected, chunk.in_remain(),
                                cspupdur.SystemParam());
                            throw Error(ERR_RAIL_PDU_TRUNCATED);
                        }

                        uint8_t Body = chunk.in_uint8();

                        if (Body) {
                            LOG(LOG_INFO,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "Menu Access Keys are always underlined.");
                        }
                        else {
                            LOG(LOG_INFO,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "Menu Access Keys are underlined only when the menu is activated by the keyboard.");
                        }
                    }
                    break;

                    case SPI_SETKEYBOARDPREF:
                    {
                        const unsigned expected = 1 /* Body(1) */;
                        if (!chunk.in_check_rem(expected)) {
                            LOG(LOG_ERR,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "expected=%u remains=%zu (0x%04X)",
                                expected, chunk.in_remain(),
                                cspupdur.SystemParam());
                            throw Error(ERR_RAIL_PDU_TRUNCATED);
                        }

                        uint8_t Body = chunk.in_uint8();

                        if (Body) {
                            LOG(LOG_INFO,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "The user prefers the keyboard over mouse.");
                        }
                        else {
                            LOG(LOG_INFO,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "The user does not prefer the keyboard over mouse.");
                        }
                    }
                    break;

                    case SPI_SETMOUSEBUTTONSWAP:
                    {
                        const unsigned expected = 1 /* Body(1) */;
                        if (!chunk.in_check_rem(expected)) {
                            LOG(LOG_ERR,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "expected=%u remains=%zu (0x%04X)",
                                expected, chunk.in_remain(),
                                cspupdur.SystemParam());
                            throw Error(ERR_RAIL_PDU_TRUNCATED);
                        }

                        uint8_t Body = chunk.in_uint8();

                        if (Body) {
                            LOG(LOG_INFO,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "Swaps the meaning of the left and right mouse buttons.");
                        }
                        else {
                            LOG(LOG_INFO,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "Restores the meaning of the left and right mouse buttons to their original meanings.");
                        }
                    }
                    break;

                    case SPI_SETWORKAREA:
                    {
                        const unsigned expected = 8 /* Body(8) */;
                        if (!chunk.in_check_rem(expected)) {
                            LOG(LOG_ERR,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "expected=%u remains=%zu (0x%04X)",
                                expected, chunk.in_remain(),
                                cspupdur.SystemParam());
                            throw Error(ERR_RAIL_PDU_TRUNCATED);
                        }

                        uint16_t Left   = chunk.in_uint16_le();
                        uint16_t Top    = chunk.in_uint16_le();
                        uint16_t Right  = chunk.in_uint16_le();
                        uint16_t Bottom = chunk.in_uint16_le();

                        LOG(LOG_INFO,
                            "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                "work area in virtual screen coordinates is (left=%u top=%u right=%u bottom=%u).",
                            Left, Top, Right, Bottom);
                    }
                    break;

                    case RAIL_SPI_DISPLAYCHANGE:
                    {
                        const unsigned expected = 8 /* Body(8) */;
                        if (!chunk.in_check_rem(expected)) {
                            LOG(LOG_ERR,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "expected=%u remains=%zu (0x%04X)",
                                expected, chunk.in_remain(),
                                cspupdur.SystemParam());
                            throw Error(ERR_RAIL_PDU_TRUNCATED);
                        }

                        uint16_t Left   = chunk.in_uint16_le();
                        uint16_t Top    = chunk.in_uint16_le();
                        uint16_t Right  = chunk.in_uint16_le();
                        uint16_t Bottom = chunk.in_uint16_le();

                        LOG(LOG_INFO,
                            "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                "New display resolution in virtual screen coordinates is (left=%u top=%u right=%u bottom=%u).",
                            Left, Top, Right, Bottom);
                    }
                    break;

                    case RAIL_SPI_TASKBARPOS:
                    {
                        const unsigned expected = 8 /* Body(8) */;
                        if (!chunk.in_check_rem(expected)) {
                            LOG(LOG_ERR,
                                "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                    "expected=%u remains=%zu (0x%04X)",
                                expected, chunk.in_remain(),
                                cspupdur.SystemParam());
                            throw Error(ERR_RAIL_PDU_TRUNCATED);
                        }

                        uint16_t Left   = chunk.in_uint16_le();
                        uint16_t Top    = chunk.in_uint16_le();
                        uint16_t Right  = chunk.in_uint16_le();
                        uint16_t Bottom = chunk.in_uint16_le();

                        LOG(LOG_INFO,
                            "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                "Size of the client taskbar is (left=%u top=%u right=%u bottom=%u).",
                            Left, Top, Right, Bottom);
                    }
                    break;

                    case SPI_SETHIGHCONTRAST:
                    {
                        HighContrastSystemInformationStructure_Recv hcsisr(chunk);

                        LOG(LOG_INFO,
                            "mod_rdp::send_to_mod_rail_channel: Client System Parameters Update PDU - "
                                "parameters for the high-contrast accessibility feature, Flags=0x%X, ColorScheme=\"%s\".",
                            hcsisr.Flags(), hcsisr.ColorScheme());
                    }
                    break;
                }
            }
            break;

            case TS_RAIL_ORDER_CLIENTSTATUS:
            {
                ClientInformationPDU_Recv cipdur(chunk);

                LOG(LOG_INFO,
                    "mod_rdp::send_to_mod_rail_channel: Client Information PDU - Flags=0x%08X",
                    cipdur.Flags());
            }
            break;

            case TS_RAIL_ORDER_HANDSHAKE:
            {
                HandshakePDU_Recv hpdur(chunk);

                LOG(LOG_INFO,
                    "mod_rdp::send_to_mod_rail_channel: Handshake PDU - buildNumber=%u",
                    hpdur.buildNumber());
            }
            break;

            default:
                LOG(LOG_INFO,
                    "mod_rdp::send_to_mod_rail_channel: undecoded PDU - orderType=%u orderLength=%u",
                    orderType, orderLength);
            break;
        }

        this->send_to_channel(*rail_channel, saved_chunk_p, chunk.get_capacity(), length, flags);
    }   // send_to_mod_rail_channel

private:
    void send_to_mod_rdpdr_channel(const CHANNELS::ChannelDef * rdpdr_channel,
                                   InStream & chunk, size_t length, uint32_t flags) {
        if (this->authorization_channels.rdpdr_type_all_is_authorized() &&
            !this->file_system_drive_manager.HasManagedDrive()) {
            if (this->verbose && (flags & CHANNELS::CHANNEL_FLAG_LAST)) {
                LOG(LOG_INFO,
                    "mod_rdp::send_to_mod_rdpdr_channel: "
                        "send Chunked Virtual Channel Data transparently.");
            }

            this->send_to_channel(*rdpdr_channel, chunk.get_data(), chunk.get_capacity(), length, flags);
            return;
        }

        BaseVirtualChannel& channel = this->get_file_system_virtual_channel();

        channel.process_client_message(length, flags, chunk.get_current(), chunk.in_remain());
    }

public:
    // Method used by session to transmit sesman answer for auth_channel
    void send_auth_channel_data(const char * string_data) override {
        //if (strncmp("Error", string_data, 5)) {
        //    this->auth_channel_state = 1; // session started
        //}

        CHANNELS::VirtualChannelPDU virtual_channel_pdu;

        StaticOutStream<65536> stream_data;
        uint32_t data_size = std::min(::strlen(string_data) + 1, stream_data.get_capacity());

        stream_data.out_copy_bytes(string_data, data_size);

        virtual_channel_pdu.send_to_server( this->nego.trans, this->encrypt, this->encryptionLevel
                            , this->userid, this->auth_channel_chanid
                            , stream_data.get_offset()
                            , this->auth_channel_flags
                            , stream_data.get_data()
                            , stream_data.get_offset());
    }

private:
    void send_to_channel(
        const CHANNELS::ChannelDef & channel,
        uint8_t const * chunk, std::size_t chunk_size,
        size_t length, uint32_t flags
    ) {
        if (this->verbose & 16) {
            LOG( LOG_INFO, "mod_rdp::send_to_channel length=%zu chunk_size=%zu", length, chunk_size);
            channel.log(-1u);
        }

        if (channel.flags & GCC::UserData::CSNet::CHANNEL_OPTION_SHOW_PROTOCOL) {
            flags |= CHANNELS::CHANNEL_FLAG_SHOW_PROTOCOL;
        }

        if (chunk_size <= CHANNELS::CHANNEL_CHUNK_LENGTH) {
            CHANNELS::VirtualChannelPDU virtual_channel_pdu;

            virtual_channel_pdu.send_to_server( this->nego.trans, this->encrypt, this->encryptionLevel
                                              , this->userid, channel.chanid, length, flags, chunk, chunk_size);
        }
        else {
            uint8_t const * virtual_channel_data = chunk;
            size_t          remaining_data_length = length;

            auto get_channel_control_flags = [] (uint32_t flags, size_t data_length,
                                                 size_t remaining_data_length,
                                                 size_t virtual_channel_data_length) -> uint32_t {
                if (remaining_data_length == data_length) {
                    return (flags & (~CHANNELS::CHANNEL_FLAG_LAST));
                }
                else if (remaining_data_length == virtual_channel_data_length) {
                    return (flags & (~CHANNELS::CHANNEL_FLAG_FIRST));
                }

                return (flags & (~(CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST)));
            };

            do {
                const size_t virtual_channel_data_length =
                    std::min<size_t>(remaining_data_length, CHANNELS::CHANNEL_CHUNK_LENGTH);

                CHANNELS::VirtualChannelPDU virtual_channel_pdu;

                virtual_channel_pdu.send_to_server( this->nego.trans, this->encrypt, this->encryptionLevel
                                                  , this->userid, channel.chanid, length
                                                  , get_channel_control_flags(flags, length, remaining_data_length, virtual_channel_data_length)
                                                  , virtual_channel_data, virtual_channel_data_length);

                remaining_data_length -= virtual_channel_data_length;
                virtual_channel_data  += virtual_channel_data_length;
            }
            while (remaining_data_length);
        }

        if (this->verbose & 16) {
            LOG(LOG_INFO, "mod_rdp::send_to_channel done");
        }
    }

    template<class... WriterData>
    void send_data_request(uint16_t channelId, WriterData... writer_data) {
        if (this->verbose & 16) {
            LOG(LOG_INFO, "send data request");
        }

        write_packets(
            this->nego.trans,
#ifdef IN_IDE_PARSER
            writer_data,
#else
            writer_data...,
#endif
            [this, channelId](StreamSize<256>, OutStream & mcs_header, std::size_t packet_size) {
                MCS::SendDataRequest_Send mcs(
                    static_cast<OutPerStream&>(mcs_header), this->userid,
                    channelId, 1, 3, packet_size, MCS::PER_ENCODING
                );
                (void)mcs;
            },
            write_x224_dt_tpdu_fn{}
        );

        if (this->verbose & 16) {
            LOG(LOG_INFO, "send data request done");
        }
    }

    template<class... WriterData>
    void send_data_request_ex(uint16_t channelId, WriterData ... writer_data) {
        this->send_data_request(
            channelId,
            writer_data...,
            write_sec_send_fn{0, this->encrypt, this->encryptionLevel}
        );
    }

public:
    void draw_event(time_t now) override {
        if (!this->event.waked_up_by_time &&
            (!this->enable_session_probe || !this->session_probe_event.set_state || !this->session_probe_event.waked_up_by_time)) {
            try{
                char * hostname = this->hostname;

                switch (this->state){
                case MOD_RDP_NEGO:
                    if (this->verbose & 1){
                        LOG(LOG_INFO, "mod_rdp::Early TLS Security Exchange");
                    }
                    switch (this->nego.state){
                    default:
                        this->nego.server_event(
                                this->server_cert_store,
                                this->server_cert_check,
                                this->server_notifier,
                                this->certif_path
                            );
                        break;
                    case RdpNego::NEGO_STATE_FINAL:
                        // Basic Settings Exchange
                        // -----------------------

                        // Basic Settings Exchange: Basic settings are exchanged between the client and
                        // server by using the MCS Connect Initial and MCS Connect Response PDUs. The
                        // Connect Initial PDU contains a GCC Conference Create Request, while the
                        // Connect Response PDU contains a GCC Conference Create Response.

                        // These two Generic Conference Control (GCC) packets contain concatenated
                        // blocks of settings data (such as core data, security data and network data)
                        // which are read by client and server


                        // Client                                                     Server
                        //    |--------------MCS Connect Initial PDU with-------------> |
                        //                   GCC Conference Create Request
                        //    | <------------MCS Connect Response PDU with------------- |
                        //                   GCC conference Create Response

                        /* Generic Conference Control (T.124) ConferenceCreateRequest */
                        write_packets(
                            this->nego.trans,
                            [this, &hostname](StreamSize<65536-1024>, OutStream & stream) {
                                // ------------------------------------------------------------
                                GCC::UserData::CSCore cs_core;
                                cs_core.version = this->use_rdp5?0x00080004:0x00080001;
                                cs_core.desktopWidth = this->front_width;
                                cs_core.desktopHeight = this->front_height;
                                //cs_core.highColorDepth = this->front_bpp;
                                cs_core.highColorDepth = ((this->front_bpp == 32)
                                    ? uint16_t(GCC::UserData::HIGH_COLOR_24BPP) : this->front_bpp);
                                cs_core.keyboardLayout = this->keylayout;
                                if (this->front_bpp == 32) {
                                    cs_core.supportedColorDepths = 15;
                                    cs_core.earlyCapabilityFlags |= GCC::UserData::RNS_UD_CS_WANT_32BPP_SESSION;
                                }

                                uint16_t hostlen = strlen(hostname);
                                uint16_t maxhostlen = std::min(uint16_t(15), hostlen);
                                for (size_t i = 0; i < maxhostlen ; i++){
                                    cs_core.clientName[i] = hostname[i];
                                }
                                memset(&(cs_core.clientName[hostlen]), 0, 16-hostlen);

                                if (this->nego.tls){
                                    cs_core.serverSelectedProtocol = this->nego.selected_protocol;
                                }
                                if (this->verbose & 1) {
                                    cs_core.log("Sending to Server");
                                }
                                cs_core.emit(stream);
                                // ------------------------------------------------------------

                                GCC::UserData::CSCluster cs_cluster;
                                TODO("CGR: values used for setting console_session looks crazy. It's old code and actual validity of these values should be checked. It should only be about REDIRECTED_SESSIONID_FIELD_VALID and shouldn't touch redirection version. Shouldn't it ?");
                                if (this->server_redirection_support) {
                                    LOG(LOG_INFO, "CS_Cluster: Server Redirection Supported");
                                    if (!this->nego.tls){
                                        cs_cluster.flags |= GCC::UserData::CSCluster::REDIRECTION_SUPPORTED;
                                        cs_cluster.flags |= (2 << 2); // REDIRECTION V3
                                    } else {
                                        cs_cluster.flags |= GCC::UserData::CSCluster::REDIRECTION_SUPPORTED;
                                        cs_cluster.flags |= (3 << 2);  // REDIRECTION V4
                                    }
                                    if (this->redir_info.valid) {
                                        cs_cluster.flags |= GCC::UserData::CSCluster::REDIRECTED_SESSIONID_FIELD_VALID;
                                        cs_cluster.redirectedSessionID = this->redir_info.session_id;
                                        LOG(LOG_INFO, "Effective Redirection SessionId=%u",
                                            cs_cluster.redirectedSessionID);
                                    }
                                }
                                if (this->console_session) {
                                    cs_cluster.flags |= GCC::UserData::CSCluster::REDIRECTED_SESSIONID_FIELD_VALID;
                                }
                                // if (!this->nego.tls){
                                //     if (this->console_session){
                                //         cs_cluster.flags = GCC::UserData::CSCluster::REDIRECTED_SESSIONID_FIELD_VALID | (3 << 2) ; // REDIRECTION V4
                                //     }
                                //     else {
                                //         cs_cluster.flags = GCC::UserData::CSCluster::REDIRECTION_SUPPORTED            | (2 << 2) ; // REDIRECTION V3
                                //     }
                                //     }
                                // else {
                                //     cs_cluster.flags = GCC::UserData::CSCluster::REDIRECTION_SUPPORTED * ((3 << 2)|1);  // REDIRECTION V4
                                //     if (this->console_session){
                                //         cs_cluster.flags |= GCC::UserData::CSCluster::REDIRECTED_SESSIONID_FIELD_VALID ;
                                //     }
                                // }
                                if (this->verbose & 1) {
                                    cs_cluster.log("Sending to server");
                                }
                                cs_cluster.emit(stream);
                                // ------------------------------------------------------------
                                GCC::UserData::CSSecurity cs_security;
                                if (this->verbose & 1) {
                                    cs_security.log("Sending to server");
                                }
                                cs_security.emit(stream);
                                // ------------------------------------------------------------

                                const CHANNELS::ChannelDefArray & channel_list = this->front.get_channel_list();
                                size_t num_channels = channel_list.size();
                                if ((num_channels > 0) || this->enable_auth_channel ||
                                    this->file_system_drive_manager.HasManagedDrive()) {
                                    /* Here we need to put channel information in order
                                    to redirect channel data
                                    from client to server passing through the "proxy" */
                                    GCC::UserData::CSNet cs_net;
                                    cs_net.channelCount = num_channels;
                                    bool has_rdpdr_channel  = false;
                                    bool has_rdpsnd_channel = false;
                                    for (size_t index = 0; index < num_channels; index++) {
                                        const CHANNELS::ChannelDef & channel_item = channel_list[index];
                                        if (this->authorization_channels.is_authorized(channel_item.name) ||
                                            ((!strcmp(channel_item.name, channel_names::rdpdr) ||
                                              !strcmp(channel_item.name, channel_names::rdpsnd)) &&
                                            this->file_system_drive_manager.HasManagedDrive())
                                        ) {
                                            if (!strcmp(channel_item.name, channel_names::rdpdr)) {
                                                has_rdpdr_channel = true;
                                            }
                                            else if (!strcmp(channel_item.name, channel_names::rdpsnd)) {
                                                has_rdpsnd_channel = true;
                                            }
                                            memcpy(cs_net.channelDefArray[index].name, channel_item.name, 8);
                                        }
                                        else {
                                            memcpy(cs_net.channelDefArray[index].name, "\0\0\0\0\0\0\0", 8);
                                        }
                                        cs_net.channelDefArray[index].options = channel_item.flags;
                                        CHANNELS::ChannelDef def;
                                        memcpy(def.name, cs_net.channelDefArray[index].name, 8);
                                        def.flags = channel_item.flags;
                                        if (this->verbose & 16) {
                                            def.log(index);
                                        }
                                        this->mod_channel_list.push_back(def);
                                    }

                                    // Inject a new channel for file system virtual channel (rdpdr)
                                    if (!has_rdpdr_channel && this->file_system_drive_manager.HasManagedDrive()) {
                                        ::snprintf(cs_net.channelDefArray[cs_net.channelCount].name,
                                                sizeof(cs_net.channelDefArray[cs_net.channelCount].name),
                                                "%s", channel_names::rdpdr);
                                        cs_net.channelDefArray[cs_net.channelCount].options =
                                            GCC::UserData::CSNet::CHANNEL_OPTION_INITIALIZED
                                            | GCC::UserData::CSNet::CHANNEL_OPTION_COMPRESS_RDP;
                                        CHANNELS::ChannelDef def;
                                        ::snprintf(def.name, sizeof(def.name), "%s", channel_names::rdpdr);
                                        def.flags = cs_net.channelDefArray[cs_net.channelCount].options;
                                        if (this->verbose & 16){
                                            def.log(cs_net.channelCount);
                                        }
                                        this->mod_channel_list.push_back(def);
                                        cs_net.channelCount++;
                                    }

                                    // The RDPDR channel advertised by the client is ONLY accepted by the RDP
                                    //  server 2012 if the RDPSND channel is also advertised.
                                    if (this->file_system_drive_manager.HasManagedDrive() &&
                                        !has_rdpsnd_channel) {
                                        ::snprintf(cs_net.channelDefArray[cs_net.channelCount].name,
                                                sizeof(cs_net.channelDefArray[cs_net.channelCount].name),
                                                "%s", channel_names::rdpsnd);
                                        cs_net.channelDefArray[cs_net.channelCount].options =
                                            GCC::UserData::CSNet::CHANNEL_OPTION_INITIALIZED
                                            | GCC::UserData::CSNet::CHANNEL_OPTION_COMPRESS_RDP;
                                        CHANNELS::ChannelDef def;
                                        ::snprintf(def.name, sizeof(def.name), "%s", channel_names::rdpsnd);
                                        def.flags = cs_net.channelDefArray[cs_net.channelCount].options;
                                        if (this->verbose & 16){
                                            def.log(cs_net.channelCount);
                                        }
                                        this->mod_channel_list.push_back(def);
                                        cs_net.channelCount++;
                                    }

                                    // Inject a new channel for auth_channel virtual channel (wablauncher)
                                    if (this->enable_auth_channel) {
                                        REDASSERT(this->auth_channel[0]);
                                        memcpy(cs_net.channelDefArray[cs_net.channelCount].name, this->auth_channel, 8);
                                        cs_net.channelDefArray[cs_net.channelCount].options =
                                            GCC::UserData::CSNet::CHANNEL_OPTION_INITIALIZED;
                                        CHANNELS::ChannelDef def;
                                        memcpy(def.name, this->auth_channel, 8);
                                        def.flags = cs_net.channelDefArray[cs_net.channelCount].options;
                                        if (this->verbose & 16){
                                            def.log(cs_net.channelCount);
                                        }
                                        this->mod_channel_list.push_back(def);
                                        cs_net.channelCount++;
                                    }

                                    if (this->enable_session_probe) {
                                        const char * session_probe_channel_name = "sespro\0\0";
                                        memcpy(cs_net.channelDefArray[cs_net.channelCount].name, session_probe_channel_name, 8);
                                        cs_net.channelDefArray[cs_net.channelCount].options =
                                            GCC::UserData::CSNet::CHANNEL_OPTION_INITIALIZED;
                                        CHANNELS::ChannelDef def;
                                        memcpy(def.name, session_probe_channel_name, 8);
                                        def.flags = cs_net.channelDefArray[cs_net.channelCount].options;
                                        if (this->verbose & 16){
                                            def.log(cs_net.channelCount);
                                        }
                                        this->mod_channel_list.push_back(def);
                                        cs_net.channelCount++;
                                    }

                                    if (this->verbose & 1) {
                                        cs_net.log("Sending to server");
                                    }
                                    cs_net.emit(stream);
                                }
                            },
                            [this](StreamSize<256>, OutStream & gcc_header, std::size_t packet_size) {
                                GCC::Create_Request_Send(
                                    static_cast<OutPerStream&>(gcc_header),
                                    packet_size
                                );
                            },
                            [this](StreamSize<256>, OutStream & mcs_header, std::size_t packet_size) {
                                MCS::CONNECT_INITIAL_Send mcs(mcs_header, packet_size, MCS::BER_ENCODING);
                                (void)mcs;
                            },
                            write_x224_dt_tpdu_fn{}
                        );

                        this->state = MOD_RDP_BASIC_SETTINGS_EXCHANGE;
                        break;
                    }
                    break;

                case MOD_RDP_BASIC_SETTINGS_EXCHANGE:
                    if (this->verbose & 1){
                        LOG(LOG_INFO, "mod_rdp::Basic Settings Exchange");
                    }
                    {
                        constexpr std::size_t array_size = 65536;
                        uint8_t array[array_size];
                        uint8_t * end = array;
                        X224::RecvFactory f(this->nego.trans, &end, array_size);
                        InStream x224_data(array, end - array);
                        X224::DT_TPDU_Recv x224(x224_data);

                        MCS::CONNECT_RESPONSE_PDU_Recv mcs(x224.payload, MCS::BER_ENCODING);

                        GCC::Create_Response_Recv gcc_cr(mcs.payload);

                        while (gcc_cr.payload.in_check_rem(4)) {

                            GCC::UserData::RecvFactory f(gcc_cr.payload);
                            switch (f.tag) {
                            case SC_CORE:
                                {
                                    GCC::UserData::SCCore sc_core;
                                    sc_core.recv(f.payload);
                                    if (this->verbose & 1) {
                                        sc_core.log("Received from server");
                                    }
                                    if (0x0080001 == sc_core.version){ // can't use rdp5
                                        this->use_rdp5 = 0;
                                    }
                                }
                                break;
                            case SC_SECURITY:
                                {
                                    GCC::UserData::SCSecurity sc_sec1;
                                    sc_sec1.recv(f.payload);
                                    if (this->verbose & 1) {
                                        sc_sec1.log("Received from server");
                                    }

                                    this->encryptionLevel = sc_sec1.encryptionLevel;
                                    this->encryptionMethod = sc_sec1.encryptionMethod;
                                    if (sc_sec1.encryptionLevel == 0
                                        &&  sc_sec1.encryptionMethod == 0) { /* no encryption */
                                        LOG(LOG_INFO, "No encryption");
                                    }
                                    else {

                                        uint8_t serverRandom[SEC_RANDOM_SIZE] = {};
                                        uint8_t modulus[SEC_MAX_MODULUS_SIZE];
                                        memset(modulus, 0, sizeof(modulus));
                                        uint8_t exponent[SEC_EXPONENT_SIZE];
                                        memset(exponent, 0, sizeof(exponent));

                                        memcpy(serverRandom, sc_sec1.serverRandom, sc_sec1.serverRandomLen);

                                        // serverCertificate (variable): The variable-length certificate containing the
                                        //  server's public key information. The length in bytes is given by the
                                        // serverCertLen field. If the encryptionMethod and encryptionLevel fields are
                                        // both set to 0 then this field MUST NOT be present.

                                        /* RSA info */
                                        if (sc_sec1.dwVersion == GCC::UserData::SCSecurity::CERT_CHAIN_VERSION_1) {
                                            memcpy(exponent, sc_sec1.proprietaryCertificate.RSAPK.pubExp, SEC_EXPONENT_SIZE);
                                            memcpy(modulus, sc_sec1.proprietaryCertificate.RSAPK.modulus,
                                                   sc_sec1.proprietaryCertificate.RSAPK.keylen - SEC_PADDING_SIZE);

                                            this->server_public_key_len = sc_sec1.proprietaryCertificate.RSAPK.keylen - SEC_PADDING_SIZE;

                                        }
                                        else {

                                            uint32_t certcount = sc_sec1.x509.certCount;
                                            if (certcount < 2){
                                                LOG(LOG_ERR, "Server didn't send enough X509 certificates");
                                                throw Error(ERR_SEC);
                                            }

                                            uint32_t cert_len = sc_sec1.x509.cert[certcount - 1].len;
                                            X509 *cert =  sc_sec1.x509.cert[certcount - 1].cert;
                                            (void)cert_len;

                                            TODO("CGR: Currently, we don't use the CA Certificate, we should"
                                                 "*) Verify the server certificate (server_cert) with the CA certificate."
                                                 "*) Store the CA Certificate with the hostname of the server we are connecting"
                                                 " to as key, and compare it when we connect the next time, in order to prevent"
                                                 " MITM-attacks.")

                                                /* By some reason, Microsoft sets the OID of the Public RSA key to
                                                   the oid for "MD5 with RSA Encryption" instead of "RSA Encryption"

                                                   Kudos to Richard Levitte for the following (. intuitive .)
                                                   lines of code that resets the OID and let's us extract the key. */

                                                int nid = OBJ_obj2nid(cert->cert_info->key->algor->algorithm);
                                            if ((nid == NID_md5WithRSAEncryption) || (nid == NID_shaWithRSAEncryption)){
                                                ASN1_OBJECT_free(cert->cert_info->key->algor->algorithm);
                                                cert->cert_info->key->algor->algorithm = OBJ_nid2obj(NID_rsaEncryption);
                                            }

                                            EVP_PKEY * epk = X509_get_pubkey(cert);
                                            if (nullptr == epk){
                                                LOG(LOG_ERR, "Failed to extract public key from certificate\n");
                                                throw Error(ERR_SEC);
                                            }

                                            TODO("see possible factorisation with ssl_calls.hpp/ssllib::rsa_encrypt")
                                            RSA * server_public_key = EVP_PKEY_get1_RSA(epk);
                                            EVP_PKEY_free(epk);
                                            this->server_public_key_len = RSA_size(server_public_key);

                                            if (nullptr == server_public_key){
                                                LOG(LOG_ERR, "Failed to parse X509 server key");
                                                throw Error(ERR_SEC);
                                            }

                                            if ((this->server_public_key_len < SEC_MODULUS_SIZE) ||
                                                (this->server_public_key_len > SEC_MAX_MODULUS_SIZE)){
                                                LOG(LOG_ERR, "Wrong server public key size (%u bits)", this->server_public_key_len * 8);
                                                throw Error(ERR_SEC_PARSE_CRYPT_INFO_MOD_SIZE_NOT_OK);
                                            }

                                            if ((BN_num_bytes(server_public_key->e) > SEC_EXPONENT_SIZE)
                                                ||  (BN_num_bytes(server_public_key->n) > SEC_MAX_MODULUS_SIZE)){
                                                LOG(LOG_ERR, "Failed to extract RSA exponent and modulus");
                                                throw Error(ERR_SEC);
                                            }
                                            int len_e = BN_bn2bin(server_public_key->e, exponent);
                                            reverseit(exponent, len_e);
                                            int len_n = BN_bn2bin(server_public_key->n, modulus);
                                            reverseit(modulus, len_n);
                                            RSA_free(server_public_key);
                                        }

                                        uint8_t client_random[SEC_RANDOM_SIZE];
                                        memset(client_random, 0, sizeof(SEC_RANDOM_SIZE));

                                        /* Generate a client random, and determine encryption keys */
                                        this->gen.random(client_random, SEC_RANDOM_SIZE);

                                        ssllib ssl;

                                        ssl.rsa_encrypt(client_crypt_random, client_random, SEC_RANDOM_SIZE, this->server_public_key_len, modulus, exponent);
                                        SEC::KeyBlock key_block(client_random, serverRandom);
                                        memcpy(encrypt.sign_key, key_block.blob0, 16);
                                        if (sc_sec1.encryptionMethod == 1){
                                            ssl.sec_make_40bit(encrypt.sign_key);
                                        }
                                        this->decrypt.generate_key(key_block.key1, sc_sec1.encryptionMethod);
                                        this->encrypt.generate_key(key_block.key2, sc_sec1.encryptionMethod);
                                    }
                                }
                                break;
                            case SC_NET:
                                {
                                    GCC::UserData::SCNet sc_net;
                                    sc_net.recv(f.payload, this->bogus_sc_net_size);

                                    /* We assume that the channel_id array is confirmed in the same order
                                       that it has been sent. If there are any channels not confirmed, they're
                                       going to be the last channels on the array sent in MCS Connect Initial */
                                    if (this->verbose & 16){
                                        LOG(LOG_INFO, "server_channels_count=%" PRIu16 " sent_channels_count=%zu",
                                            sc_net.channelCount,
                                            mod_channel_list.size());
                                    }
                                    for (uint32_t index = 0; index < sc_net.channelCount; index++) {
                                        if (this->verbose & 16){
                                            this->mod_channel_list[index].log(index);
                                        }
                                        this->mod_channel_list.set_chanid(index, sc_net.channelDefArray[index].id);
                                    }
                                    if (this->verbose & 1) {
                                        sc_net.log("Received from server");
                                    }
                                }
                                break;
                            default:
                                LOG(LOG_ERR, "unsupported GCC UserData response tag 0x%x", f.tag);
                                throw Error(ERR_GCC);
                            }
                        }
                        if (gcc_cr.payload.in_check_rem(1)) {
                            LOG(LOG_ERR, "Error while parsing GCC UserData : short header");
                            throw Error(ERR_GCC);
                        }

                    }

                    if (this->verbose & (1|16)){
                        LOG(LOG_INFO, "mod_rdp::Channel Connection");
                    }

                    // Channel Connection
                    // ------------------
                    // Channel Connection: The client sends an MCS Erect Domain Request PDU,
                    // followed by an MCS Attach User Request PDU to attach the primary user
                    // identity to the MCS domain.

                    // The server responds with an MCS Attach User Response PDU containing the user
                    // channel ID.

                    // The client then proceeds to join the :
                    // - user channel,
                    // - the input/output (I/O) channel
                    // - and all of the static virtual channels

                    // (the I/O and static virtual channel IDs are obtained from the data embedded
                    //  in the GCC packets) by using multiple MCS Channel Join Request PDUs.

                    // The server confirms each channel with an MCS Channel Join Confirm PDU.
                    // (The client only sends a Channel Join Request after it has received the
                    // Channel Join Confirm for the previously sent request.)

                    // From this point, all subsequent data sent from the client to the server is
                    // wrapped in an MCS Send Data Request PDU, while data sent from the server to
                    //  the client is wrapped in an MCS Send Data Indication PDU. This is in
                    // addition to the data being wrapped by an X.224 Data PDU.

                    // Client                                                     Server
                    //    |-------MCS Erect Domain Request PDU--------------------> |
                    //    |-------MCS Attach User Request PDU---------------------> |

                    //    | <-----MCS Attach User Confirm PDU---------------------- |

                    //    |-------MCS Channel Join Request PDU--------------------> |
                    //    | <-----MCS Channel Join Confirm PDU--------------------- |

                    if (this->verbose & 1){
                        LOG(LOG_INFO, "Send MCS::ErectDomainRequest");
                    }
                    write_packets(
                        this->nego.trans,
                        [](StreamSize<256>, OutStream & mcs_header){
                            MCS::ErectDomainRequest_Send mcs(
                                static_cast<OutPerStream&>(mcs_header),
                                0, 0, MCS::PER_ENCODING
                            );
                            (void)mcs;
                        },
                        write_x224_dt_tpdu_fn{}
                    );
                    if (this->verbose & 1){
                        LOG(LOG_INFO, "Send MCS::AttachUserRequest");
                    }
                    write_packets(
                        this->nego.trans,
                        [](StreamSize<256>, OutStream & mcs_data){
                            MCS::AttachUserRequest_Send mcs(mcs_data, MCS::PER_ENCODING);
                            (void)mcs;
                        },
                        write_x224_dt_tpdu_fn{}
                    );
                    this->state = MOD_RDP_CHANNEL_CONNECTION_ATTACH_USER;
                    break;

                case MOD_RDP_CHANNEL_CONNECTION_ATTACH_USER:
                    if (this->verbose & 1){
                        LOG(LOG_INFO, "mod_rdp::Channel Connection Attach User");
                    }
                    {
                        {
                            constexpr size_t array_size = AUTOSIZE;
                            uint8_t array[array_size];
                            uint8_t * end = array;
                            X224::RecvFactory f(this->nego.trans, &end, array_size);
                            InStream stream(array, end - array);
                            X224::DT_TPDU_Recv x224(stream);
                            InStream & mcs_cjcf_data = x224.payload;
                            MCS::AttachUserConfirm_Recv mcs(mcs_cjcf_data, MCS::PER_ENCODING);
                            if (mcs.initiator_flag){
                                this->userid = mcs.initiator;
                            }
                        }

                        {
                            size_t num_channels = this->mod_channel_list.size();
                            uint16_t channels_id[CHANNELS::MAX_STATIC_VIRTUAL_CHANNELS + 2];
                            channels_id[0] = this->userid + GCC::MCS_USERCHANNEL_BASE;
                            channels_id[1] = GCC::MCS_GLOBAL_CHANNEL;
                            for (size_t index = 0; index < num_channels; index++){
                                channels_id[index+2] = this->mod_channel_list[index].chanid;
                            }

                            for (size_t index = 0; index < num_channels+2; index++) {
                                if (this->verbose & 16){
                                    LOG(LOG_INFO, "cjrq[%zu] = %" PRIu16, index, channels_id[index]);
                                }
                                write_packets(
                                    this->nego.trans,
                                    [this, &channels_id, index](StreamSize<256>, OutStream & mcs_cjrq_data){
                                        MCS::ChannelJoinRequest_Send mcs(
                                            mcs_cjrq_data, this->userid,
                                            channels_id[index], MCS::PER_ENCODING
                                        );
                                        (void)mcs;
                                    },
                                    write_x224_dt_tpdu_fn{}
                                );
                                constexpr size_t array_size = AUTOSIZE;
                                uint8_t array[array_size];
                                uint8_t * end = array;
                                X224::RecvFactory f(this->nego.trans, &end, array_size);
                                InStream x224_data(array, end - array);

                                X224::DT_TPDU_Recv x224(x224_data);
                                InStream & mcs_cjcf_data = x224.payload;
                                MCS::ChannelJoinConfirm_Recv mcs(mcs_cjcf_data, MCS::PER_ENCODING);
                                TODO("If mcs.result is negative channel is not confirmed and should be removed from mod_channel list");
                                if (this->verbose & 16){
                                    LOG(LOG_INFO, "cjcf[%zu] = %" PRIu16, index, mcs.channelId);
                                }
                            }
                        }

                        // RDP Security Commencement
                        // -------------------------

                        // RDP Security Commencement: If standard RDP security methods are being
                        // employed and encryption is in force (this is determined by examining the data
                        // embedded in the GCC Conference Create Response packet) then the client sends
                        // a Security Exchange PDU containing an encrypted 32-byte random number to the
                        // server. This random number is encrypted with the public key of the server
                        // (the server's public key, as well as a 32-byte server-generated random
                        // number, are both obtained from the data embedded in the GCC Conference Create
                        //  Response packet).

                        // The client and server then utilize the two 32-byte random numbers to generate
                        // session keys which are used to encrypt and validate the integrity of
                        // subsequent RDP traffic.

                        // From this point, all subsequent RDP traffic can be encrypted and a security
                        // header is include " with the data if encryption is in force (the Client Info
                        // and licensing PDUs are an exception in that they always have a security
                        // header). The Security Header follows the X.224 and MCS Headers and indicates
                        // whether the attached data is encrypted.

                        // Even if encryption is in force server-to-client traffic may not always be
                        // encrypted, while client-to-server traffic will always be encrypted by
                        // Microsoft RDP implementations (encryption of licensing PDUs is optional,
                        // however).

                        // Client                                                     Server
                        //    |------Security Exchange PDU ---------------------------> |
                        if (this->verbose & 1){
                            LOG(LOG_INFO, "mod_rdp::RDP Security Commencement");
                        }

                        if (this->encryptionLevel){
                            if (this->verbose & 1){
                                LOG(LOG_INFO, "mod_rdp::SecExchangePacket keylen=%u",
                                    this->server_public_key_len);
                            }
                            this->send_data_request(
                                GCC::MCS_GLOBAL_CHANNEL,
                                dynamic_packet(this->server_public_key_len + 32, [this](OutStream & stream) {
                                    SEC::SecExchangePacket_Send mcs(
                                        stream, this->client_crypt_random, this->server_public_key_len
                                    );
                                    (void)mcs;
                                })
                            );
                        }

                        // Secure Settings Exchange
                        // ------------------------

                        // Secure Settings Exchange: Secure client data (such as the username,
                        // password and auto-reconnect cookie) is sent to the server using the Client
                        // Info PDU.

                        // Client                                                     Server
                        //    |------ Client Info PDU      ---------------------------> |

                        if (this->verbose & 1){
                            LOG(LOG_INFO, "mod_rdp::Secure Settings Exchange");
                        }

                        this->send_client_info_pdu(this->userid, this->password);

                        this->state = MOD_RDP_GET_LICENSE;
                    }
                    break;

                case MOD_RDP_GET_LICENSE:
                    if (this->verbose & 2){
                        LOG(LOG_INFO, "mod_rdp::Licensing");
                    }
                    // Licensing
                    // ---------

                    // Licensing: The goal of the licensing exchange is to transfer a
                    // license from the server to the client.

                    // The client should store this license and on subsequent
                    // connections send the license to the server for validation.
                    // However, in some situations the client may not be issued a
                    // license to store. In effect, the packets exchanged during this
                    // phase of the protocol depend on the licensing mechanisms
                    // employed by the server. Within the context of this document
                    // we will assume that the client will not be issued a license to
                    // store. For details regarding more advanced licensing scenarios
                    // that take place during the Licensing Phase, see [MS-RDPELE].

                    // Client                                                     Server
                    //    | <------ License Error PDU Valid Client ---------------- |

                    // 2.2.1.12 Server License Error PDU - Valid Client
                    // ================================================

                    // The License Error (Valid Client) PDU is an RDP Connection Sequence PDU sent
                    // from server to client during the Licensing phase of the RDP Connection
                    // Sequence (see section 1.3.1.1 for an overview of the RDP Connection Sequence
                    // phases). This licensing PDU indicates that the server will not issue the
                    // client a license to store and that the Licensing Phase has ended
                    // successfully. This is one possible licensing PDU that may be sent during the
                    // Licensing Phase (see [MS-RDPELE] section 2.2.2 for a list of all permissible
                    // licensing PDUs).

                    // tpktHeader (4 bytes): A TPKT Header, as specified in [T123] section 8.

                    // x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in [X224] section 13.7.

                    // mcsSDin (variable): Variable-length PER-encoded MCS Domain PDU (DomainMCSPDU)
                    // which encapsulates an MCS Send Data Indication structure (SDin, choice 26
                    // from DomainMCSPDU), as specified in [T125] section 11.33 (the ASN.1 structure
                    // definitions are given in [T125] section 7, parts 7 and 10). The userData
                    // field of the MCS Send Data Indication contains a Security Header and a Valid
                    // Client License Data (section 2.2.1.12.1) structure.

                    // securityHeader (variable): Security header. The format of the security header
                    // depends on the Encryption Level and Encryption Method selected by the server
                    // (sections 5.3.2 and 2.2.1.4.3).

                    // This field MUST contain one of the following headers:
                    //  - Basic Security Header (section 2.2.8.1.1.2.1) if the Encryption Level
                    // selected by the server is ENCRYPTION_LEVEL_NONE (0) or ENCRYPTION_LEVEL_LOW
                    // (1) and the embedded flags field does not contain the SEC_ENCRYPT (0x0008)
                    // flag.
                    //  - Non-FIPS Security Header (section 2.2.8.1.1.2.2) if the Encryption Method
                    // selected by the server is ENCRYPTION_METHOD_40BIT (0x00000001),
                    // ENCRYPTION_METHOD_56BIT (0x00000008), or ENCRYPTION_METHOD_128BIT
                    // (0x00000002) and the embedded flags field contains the SEC_ENCRYPT (0x0008)
                    // flag.
                    //  - FIPS Security Header (section 2.2.8.1.1.2.3) if the Encryption Method
                    // selected by the server is ENCRYPTION_METHOD_FIPS (0x00000010) and the
                    // embedded flags field contains the SEC_ENCRYPT (0x0008) flag.

                    // If the Encryption Level is set to ENCRYPTION_LEVEL_CLIENT_COMPATIBLE (2),
                    // ENCRYPTION_LEVEL_HIGH (3), or ENCRYPTION_LEVEL_FIPS (4) and the flags field
                    // of the security header does not contain the SEC_ENCRYPT (0x0008) flag (the
                    // licensing PDU is not encrypted), then the field MUST contain a Basic Security
                    // Header. This MUST be the case if SEC_LICENSE_ENCRYPT_SC (0x0200) flag was not
                    // set on the Security Exchange PDU (section 2.2.1.10).

                    // The flags field of the security header MUST contain the SEC_LICENSE_PKT
                    // (0x0080) flag (see Basic (TS_SECURITY_HEADER)).

                    // validClientLicenseData (variable): The actual contents of the License Error
                    // (Valid Client) PDU, as specified in section 2.2.1.12.1.

                    {
                        const char * hostname = this->hostname;
                        const char * username;
                        char username_a_domain[512];
                        if (this->domain[0]) {
                            snprintf(username_a_domain, sizeof(username_a_domain), "%s@%s", this->username, this->domain);
                            username = username_a_domain;
                        }
                        else {
                            username = this->username;
                        }
                        LOG(LOG_INFO, "Rdp::Get license: username=\"%s\"", username);
                        // read tpktHeader (4 bytes = 3 0 len)
                        // TPDU class 0    (3 bytes = LI F0 PDU_DT)

                        constexpr size_t array_size = AUTOSIZE;
                        uint8_t array[array_size];
                        uint8_t * end = array;
                        X224::RecvFactory f(this->nego.trans, &end, array_size);
                        InStream stream(array, end - array);
                        X224::DT_TPDU_Recv x224(stream);
                        TODO("Shouldn't we use mcs_type to manage possible Deconnection Ultimatum here")
                        //int mcs_type = MCS::peekPerEncodedMCSType(x224.payload);
                        MCS::SendDataIndication_Recv mcs(x224.payload, MCS::PER_ENCODING);

                        SEC::SecSpecialPacket_Recv sec(mcs.payload, this->decrypt, this->encryptionLevel);

                        if (sec.flags & SEC::SEC_LICENSE_PKT) {
                            LIC::RecvFactory flic(sec.payload);

                            switch (flic.tag) {
                            case LIC::LICENSE_REQUEST:
                                if (this->verbose & 2) {
                                    LOG(LOG_INFO, "Rdp::License Request");
                                }
                                {
                                    LIC::LicenseRequest_Recv lic(sec.payload);
                                    uint8_t null_data[SEC_MODULUS_SIZE];
                                    memset(null_data, 0, sizeof(null_data));
                                    /* We currently use null client keys. This is a bit naughty but, hey,
                                       the security of license negotiation isn't exactly paramount. */
                                    SEC::SessionKey keyblock(null_data, null_data, lic.server_random);

                                    /* Store first 16 bytes of session key as MAC secret */
                                    memcpy(this->lic_layer_license_sign_key, keyblock.get_MAC_salt_key(), 16);
                                    memcpy(this->lic_layer_license_key, keyblock.get_LicensingEncryptionKey(), 16);
                                }
                                this->send_data_request(
                                    GCC::MCS_GLOBAL_CHANNEL,
                                    [this, &hostname, &username](StreamSize<65535 - 1024>, OutStream & lic_data) {
                                        if (this->lic_layer_license_size > 0) {
                                            uint8_t hwid[LIC::LICENSE_HWID_SIZE];
                                            buf_out_uint32(hwid, 2);
                                            memcpy(hwid + 4, hostname, LIC::LICENSE_HWID_SIZE - 4);

                                            /* Generate a signature for the HWID buffer */
                                            uint8_t signature[LIC::LICENSE_SIGNATURE_SIZE];

                                            uint8_t lenhdr[4];
                                            buf_out_uint32(lenhdr, sizeof(hwid));

                                            Sign sign(this->lic_layer_license_sign_key, 16);
                                            sign.update(lenhdr, sizeof(lenhdr));
                                            sign.update(hwid, sizeof(hwid));

                                            assert(MD5_DIGEST_LENGTH == LIC::LICENSE_SIGNATURE_SIZE);
                                            sign.final(signature, sizeof(signature));


                                            /* Now encrypt the HWID */

                                            SslRC4 rc4;
                                            rc4.set_key(this->lic_layer_license_key, 16);

                                            // in, out
                                            rc4.crypt(LIC::LICENSE_HWID_SIZE, hwid, hwid);

                                            LIC::ClientLicenseInfo_Send(
                                                lic_data, this->use_rdp5?3:2,
                                                this->lic_layer_license_size,
                                                this->lic_layer_license_data.get(),
                                                hwid, signature
                                            );
                                        }
                                        else {
                                            LIC::NewLicenseRequest_Send(
                                                lic_data, this->use_rdp5?3:2, username, hostname
                                            );
                                        }
                                    },
                                    write_sec_send_fn{SEC::SEC_LICENSE_PKT, this->encrypt, 0}
                                );
                                break;
                            case LIC::PLATFORM_CHALLENGE:
                                if (this->verbose & 2){
                                    LOG(LOG_INFO, "Rdp::Platform Challenge");
                                }
                                {
                                    LIC::PlatformChallenge_Recv lic(sec.payload);

                                    uint8_t out_token[LIC::LICENSE_TOKEN_SIZE];
                                    uint8_t decrypt_token[LIC::LICENSE_TOKEN_SIZE];
                                    uint8_t hwid[LIC::LICENSE_HWID_SIZE];
                                    uint8_t crypt_hwid[LIC::LICENSE_HWID_SIZE];
                                    uint8_t out_sig[LIC::LICENSE_SIGNATURE_SIZE];

                                    memcpy(out_token, lic.encryptedPlatformChallenge.blob, LIC::LICENSE_TOKEN_SIZE);
                                    /* Decrypt the token. It should read TEST in Unicode. */
                                    memcpy(decrypt_token, lic.encryptedPlatformChallenge.blob, LIC::LICENSE_TOKEN_SIZE);
                                    SslRC4 rc4_decrypt_token;
                                    rc4_decrypt_token.set_key(this->lic_layer_license_key, 16);
                                    // size, in, out
                                    rc4_decrypt_token.crypt(LIC::LICENSE_TOKEN_SIZE, decrypt_token, decrypt_token);

                                    /* Generate a signature for a buffer of token and HWID */
                                    buf_out_uint32(hwid, 2);
                                    memcpy(hwid + 4, hostname, LIC::LICENSE_HWID_SIZE - 4);

                                    uint8_t sealed_buffer[LIC::LICENSE_TOKEN_SIZE + LIC::LICENSE_HWID_SIZE];
                                    memcpy(sealed_buffer, decrypt_token, LIC::LICENSE_TOKEN_SIZE);
                                    memcpy(sealed_buffer + LIC::LICENSE_TOKEN_SIZE, hwid, LIC::LICENSE_HWID_SIZE);

                                    uint8_t lenhdr[4];
                                    buf_out_uint32(lenhdr, sizeof(sealed_buffer));

                                    Sign sign(this->lic_layer_license_sign_key, 16);
                                    sign.update(lenhdr, sizeof(lenhdr));
                                    sign.update(sealed_buffer, sizeof(sealed_buffer));

                                    assert(MD5_DIGEST_LENGTH == LIC::LICENSE_SIGNATURE_SIZE);
                                    sign.final(out_sig, sizeof(out_sig));

                                    /* Now encrypt the HWID */
                                    memcpy(crypt_hwid, hwid, LIC::LICENSE_HWID_SIZE);
                                    SslRC4 rc4_hwid;
                                    rc4_hwid.set_key(this->lic_layer_license_key, 16);
                                    // size, in, out
                                    rc4_hwid.crypt(LIC::LICENSE_HWID_SIZE, crypt_hwid, crypt_hwid);

                                    this->send_data_request(
                                        GCC::MCS_GLOBAL_CHANNEL,
                                        [&, this](StreamSize<65535 - 1024>, OutStream & lic_data) {
                                            LIC::ClientPlatformChallengeResponse_Send(
                                                lic_data, this->use_rdp5?3:2, out_token, crypt_hwid, out_sig
                                            );
                                        },
                                        write_sec_send_fn{SEC::SEC_LICENSE_PKT, this->encrypt, 0}
                                    );
                                }
                                break;
                            case LIC::NEW_LICENSE:
                                {
                                    if (this->verbose & 2){
                                        LOG(LOG_INFO, "Rdp::New License");
                                    }

                                    LIC::NewLicense_Recv lic(sec.payload, this->lic_layer_license_key);

                                    TODO("CGR: Save license to keep a local copy of the license of a remote server thus avoiding to ask it every time we connect. Not obvious files is the best choice to do that");
                                        this->state = MOD_RDP_CONNECTED;

                                    LOG(LOG_WARNING, "New license not saved");
                                }
                                break;
                            case LIC::UPGRADE_LICENSE:
                                {
                                    if (this->verbose & 2){
                                        LOG(LOG_INFO, "Rdp::Upgrade License");
                                    }
                                    LIC::UpgradeLicense_Recv lic(sec.payload, this->lic_layer_license_key);

                                    LOG(LOG_WARNING, "Upgraded license not saved");
                                }
                                break;
                            case LIC::ERROR_ALERT:
                                {
                                    if (this->verbose & 2){
                                        LOG(LOG_INFO, "Rdp::Get license status");
                                    }
                                    LIC::ErrorAlert_Recv lic(sec.payload);
                                    if ((lic.validClientMessage.dwErrorCode == LIC::STATUS_VALID_CLIENT)
                                        && (lic.validClientMessage.dwStateTransition == LIC::ST_NO_TRANSITION)){
                                        this->state = MOD_RDP_CONNECTED;
                                    }
                                    else {
                                        LOG(LOG_ERR, "RDP::License Alert: error=%u transition=%u",
                                            lic.validClientMessage.dwErrorCode, lic.validClientMessage.dwStateTransition);
                                    }
                                    this->state = MOD_RDP_CONNECTED;
                                }
                                break;
                            default:
                                {
                                    LOG(LOG_ERR, "Unexpected license tag sent from server (tag = %x)", flic.tag);
                                    throw Error(ERR_SEC);
                                }
                                break;
                            }

                            if (sec.payload.get_current() != sec.payload.get_data_end()){
                                LOG(LOG_ERR, "all data should have been consumed %s:%u tag = %x", __FILE__, __LINE__, flic.tag);
                                throw Error(ERR_SEC);
                            }
                        }
                        else {
                            LOG(LOG_WARNING, "Failed to get expected license negotiation PDU");
                            hexdump(x224.payload.get_data(), x224.payload.get_capacity());
                            //throw Error(ERR_SEC);
                            this->state = MOD_RDP_CONNECTED;
                            hexdump(sec.payload.get_data(), sec.payload.get_capacity());
                        }
                    }
                    break;

                    // Capabilities Exchange
                    // ---------------------

                    // Capabilities Negotiation: The server sends the set of capabilities it
                    // supports to the client in a Demand Active PDU. The client responds with its
                    // capabilities by sending a Confirm Active PDU.

                    // Client                                                     Server
                    //    | <------- Demand Active PDU ---------------------------- |
                    //    |--------- Confirm Active PDU --------------------------> |

                    // Connection Finalization
                    // -----------------------

                    // Connection Finalization: The client and server send PDUs to finalize the
                    // connection details. The client-to-server and server-to-client PDUs exchanged
                    // during this phase may be sent concurrently as long as the sequencing in
                    // either direction is maintained (there are no cross-dependencies between any
                    // of the client-to-server and server-to-client PDUs). After the client receives
                    // the Font Map PDU it can start sending mouse and keyboard input to the server,
                    // and upon receipt of the Font List PDU the server can start sending graphics
                    // output to the client.

                    // Client                                                     Server
                    //    |----------Synchronize PDU------------------------------> |
                    //    |----------Control PDU Cooperate------------------------> |
                    //    |----------Control PDU Request Control------------------> |
                    //    |----------Persistent Key List PDU(s)-------------------> |
                    //    |----------Font List PDU--------------------------------> |

                    //    | <--------Synchronize PDU------------------------------- |
                    //    | <--------Control PDU Cooperate------------------------- |
                    //    | <--------Control PDU Granted Control------------------- |
                    //    | <--------Font Map PDU---------------------------------- |

                    // All PDU's in the client-to-server direction must be sent in the specified
                    // order and all PDU's in the server to client direction must be sent in the
                    // specified order. However, there is no requirement that client to server PDU's
                    // be sent before server-to-client PDU's. PDU's may be sent concurrently as long
                    // as the sequencing in either direction is maintained.


                    // Besides input and graphics data, other data that can be exchanged between
                    // client and server after the connection has been finalized include "
                    // connection management information and virtual channel messages (exchanged
                    // between client-side plug-ins and server-side applications).

                case MOD_RDP_CONNECTED:
                    {
                        // read tpktHeader (4 bytes = 3 0 len)
                        // TPDU class 0    (3 bytes = LI F0 PDU_DT)

                        // Detect fast-path PDU
                        constexpr std::size_t array_size = 65536;
                        uint8_t array[array_size];
                        uint8_t * end = array;
                        X224::RecvFactory fx224(this->nego.trans, &end, array_size, true);
                        InStream stream(array, end - array);

                        if (fx224.fast_path) {
                            FastPath::ServerUpdatePDU_Recv su(stream, this->decrypt, array);
                            if (this->enable_transparent_mode) {
                                //total_data_received += su.payload.size();
                                //LOG(LOG_INFO, "total_data_received=%llu", total_data_received);
                                if (this->transparent_recorder) {
                                    this->transparent_recorder->send_fastpath_data(su.payload);
                                }
                                this->front.send_fastpath_data(su.payload);

                                break;
                            }

                            while (su.payload.in_remain()) {
                                FastPath::Update_Recv upd(su.payload, &this->mppc_dec);

                                switch (upd.updateCode) {
                                case FastPath::FASTPATH_UPDATETYPE_ORDERS:
                                    this->front.begin_update();
                                    this->orders.process_orders(this->bpp, upd.payload, true, *this->gd,
                                                                this->front_width, this->front_height);
                                    this->front.end_update();

                                    if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_ORDERS"); }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_BITMAP:
                                    this->front.begin_update();
                                    this->process_bitmap_updates(upd.payload, true);
                                    this->front.end_update();

                                    if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_BITMAP"); }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_PALETTE:
                                    this->front.begin_update();
                                    this->process_palette(upd.payload, true);
                                    this->front.end_update();

                                    if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_PALETTE"); }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_SYNCHRONIZE:
                                    if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_SYNCHRONIZE"); }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_PTR_NULL:
                                    {
                                        if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_PTR_NULL"); }
                                        struct Pointer cursor;
                                        memset(cursor.mask, 0xff, sizeof(cursor.mask));
                                        this->front.server_set_pointer(cursor);
                                    }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_PTR_DEFAULT:
                                    {
                                        if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_PTR_DEFAULT"); }
                                        Pointer cursor(Pointer::POINTER_SYSTEM_DEFAULT);
                                        this->front.server_set_pointer(cursor);
                                    }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_PTR_POSITION:
                                    {
                                        if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_PTR_POSITION"); }
                                        uint16_t xPos = upd.payload.in_uint16_le();
                                        uint16_t yPos = upd.payload.in_uint16_le();
                                        this->front.update_pointer_position(xPos, yPos);
                                    }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_COLOR:
                                    this->process_color_pointer_pdu(upd.payload);

                                    if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_COLOR"); }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_POINTER:
                                    this->process_new_pointer_pdu(upd.payload);

                                    if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_POINTER"); }
                                    break;

                                case FastPath::FASTPATH_UPDATETYPE_CACHED:
                                    this->process_cached_pointer_pdu(upd.payload);

                                    if (this->verbose & 8) { LOG(LOG_INFO, "FASTPATH_UPDATETYPE_CACHED"); }
                                    break;

                                default:
                                    LOG( LOG_ERR
                                       , "mod::rdp: received unexpected fast-path PUD, updateCode = %u"
                                       , upd.updateCode);
                                    throw Error(ERR_RDP_FASTPATH);
                                }
                            }

                            TODO("Chech all data in the PDU is consumed");
                            break;
                        }

                        X224::DT_TPDU_Recv x224(stream);

                        const int mcs_type = MCS::peekPerEncodedMCSType(x224.payload);

                        if (mcs_type == MCS::MCSPDU_DisconnectProviderUltimatum){
                            LOG(LOG_INFO, "mod::rdp::DisconnectProviderUltimatum received");
                            x224.payload.rewind();
                            MCS::DisconnectProviderUltimatum_Recv mcs(x224.payload, MCS::PER_ENCODING);
                            const char * reason = MCS::get_reason(mcs.reason);
                            LOG(LOG_ERR, "mod::rdp::DisconnectProviderUltimatum: reason=%s [%d]", reason, mcs.reason);

                            if (this->acl) {
                                this->end_session_reason.clear();
                                this->end_session_message.clear();

                                this->acl->report("CLOSE_SESSION_SUCCESSFUL", "OK.");

                                this->acl->log4(false, "SESSION_DISCONNECTED_BY_TARGET");
                            }
                            throw Error(ERR_MCS_APPID_IS_MCS_DPUM);
                        }


                        MCS::SendDataIndication_Recv mcs(x224.payload, MCS::PER_ENCODING);
                        SEC::Sec_Recv sec(mcs.payload, this->decrypt, this->encryptionLevel);

                        if (mcs.channelId != GCC::MCS_GLOBAL_CHANNEL){
                            if (this->verbose & 16) {
                                LOG(LOG_INFO, "received channel data on mcs.chanid=%u", mcs.channelId);
                            }

                            int num_channel_src = this->mod_channel_list.get_index_by_id(mcs.channelId);
                            if (num_channel_src == -1) {
                                LOG(LOG_ERR, "mod::rdp::MOD_RDP_CONNECTED::Unknown Channel id=%d", mcs.channelId);
                                throw Error(ERR_CHANNEL_UNKNOWN_CHANNEL);
                            }

                            const CHANNELS::ChannelDef & mod_channel = this->mod_channel_list[num_channel_src];
                            if (this->verbose & 16) {
                                mod_channel.log(num_channel_src);
                            }

                            uint32_t length = sec.payload.in_uint32_le();
                            int flags = sec.payload.in_uint32_le();
                            size_t chunk_size = sec.payload.in_remain();

                            // If channel name is our virtual channel, then don't send data to front
                                 if (  this->enable_auth_channel
                                    && !strcmp(mod_channel.name, this->auth_channel)) {
                                this->process_auth_event(mod_channel, sec.payload, length, flags, chunk_size);
                            }
                            else if (!strcmp(mod_channel.name, "sespro")) {
                                this->process_session_probe_event(mod_channel, sec.payload, length, flags, chunk_size);
                            }
                            // Clipboard is a Clipboard PDU
                            else if (!strcmp(mod_channel.name, channel_names::cliprdr)) {
                                this->process_cliprdr_event(mod_channel, sec.payload, length, flags, chunk_size);
                            }
                            else if (!strcmp(mod_channel.name, channel_names::rail)) {
                                this->process_rail_event(mod_channel, sec.payload, length, flags, chunk_size);
                            }
                            else if (!strcmp(mod_channel.name, channel_names::rdpdr)) {
                                this->process_rdpdr_event(mod_channel, sec.payload, length, flags, chunk_size);
                            }
                            else {
                                this->send_to_front_channel(
                                    mod_channel.name, sec.payload.get_current(), length, chunk_size, flags
                                );
                            }
                            sec.payload.in_skip_bytes(sec.payload.in_remain());
                        }
                        else {
                            uint8_t const * next_packet = sec.payload.get_current();
                            while (next_packet < sec.payload.get_data_end()) {
                                sec.payload.rewind();
                                sec.payload.in_skip_bytes(next_packet - sec.payload.get_data());

                                uint8_t const * current_packet = next_packet;

                                if  (peekFlowPDU(sec.payload)){
                                    if (this->verbose & 128) {
                                        LOG(LOG_WARNING, "FlowPDU TYPE");
                                    }
                                    ShareFlow_Recv sflow(sec.payload);
                                    // ignoring
                                    // if (sctrl.flow_pdu_type == FLOW_TEST_PDU) {
                                    //     this->send_flow_response_pdu(sctrl.flow_id,
                                    //                                  sctrl.flow_number);
                                    // }
                                    next_packet = sec.payload.get_current();
                                }
                                else {
                                    ShareControl_Recv sctrl(sec.payload);
                                    next_packet += sctrl.totalLength;

                                    if (this->verbose & 128) {
                                        LOG(LOG_WARNING, "LOOPING on PDUs: %u", unsigned(sctrl.totalLength));
                                    }

                                    switch (sctrl.pduType) {
                                    case PDUTYPE_DATAPDU:
                                        if (this->verbose & 128) {
                                            LOG(LOG_WARNING, "PDUTYPE_DATAPDU");
                                        }
                                        switch (this->connection_finalization_state){
                                        case EARLY:
                                            LOG(LOG_ERR, "Rdp::finalization is early");
                                            throw Error(ERR_SEC);
                                        case WAITING_SYNCHRONIZE:
                                            if (this->verbose & 1){
                                                LOG(LOG_WARNING, "WAITING_SYNCHRONIZE");
                                            }
                                            //this->check_data_pdu(PDUTYPE2_SYNCHRONIZE);
                                            this->connection_finalization_state = WAITING_CTL_COOPERATE;
                                            {
                                                ShareData_Recv sdata(sctrl.payload, &this->mppc_dec);
                                                sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                            }
                                            break;
                                        case WAITING_CTL_COOPERATE:
                                            if (this->verbose & 1){
                                                LOG(LOG_WARNING, "WAITING_CTL_COOPERATE");
                                            }
                                            //this->check_data_pdu(PDUTYPE2_CONTROL);
                                            this->connection_finalization_state = WAITING_GRANT_CONTROL_COOPERATE;
                                            {
                                                ShareData_Recv sdata(sctrl.payload, &this->mppc_dec);
                                                sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                            }
                                            break;
                                        case WAITING_GRANT_CONTROL_COOPERATE:
                                            if (this->verbose & 1){
                                                LOG(LOG_WARNING, "WAITING_GRANT_CONTROL_COOPERATE");
                                            }
                                            //                            this->check_data_pdu(PDUTYPE2_CONTROL);
                                            this->connection_finalization_state = WAITING_FONT_MAP;
                                            {
                                                ShareData_Recv sdata(sctrl.payload, &this->mppc_dec);
                                                sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                            }
                                            break;
                                        case WAITING_FONT_MAP:
                                            if (this->verbose & 1){
                                                LOG(LOG_WARNING, "PDUTYPE2_FONTMAP");
                                            }
                                            //this->check_data_pdu(PDUTYPE2_FONTMAP);
                                            this->connection_finalization_state = UP_AND_RUNNING;

                                            if (this->acl && !this->deactivation_reactivation_in_progress) {
                                                this->acl->log4(false, "SESSION_ESTABLISHED_SUCCESSFULLY");
                                            }

                                            // Synchronize sent to indicate server the state of sticky keys (x-locks)
                                            // Must be sent at this point of the protocol (sent before, it xwould be ignored or replaced)
                                            rdp_input_synchronize(0, 0, (this->key_flags & 0x07), 0);
                                            {
                                                ShareData_Recv sdata(sctrl.payload, &this->mppc_dec);
                                                sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                            }

                                            this->deactivation_reactivation_in_progress = false;
                                            break;
                                        case UP_AND_RUNNING:
                                            if (this->enable_transparent_mode)
                                            {
                                                sec.payload.rewind();
                                                sec.payload.in_skip_bytes(current_packet - sec.payload.get_data());

                                                StaticOutStream<65535> copy_stream;
                                                copy_stream.out_copy_bytes(current_packet, next_packet - current_packet);

                                                //total_data_received += copy_stream.size();
                                                //LOG(LOG_INFO, "total_data_received=%llu", total_data_received);

                                                if (this->transparent_recorder) {
                                                    this->transparent_recorder->send_data_indication_ex(
                                                        mcs.channelId,
                                                        copy_stream.get_data(),
                                                        copy_stream.get_offset()
                                                    );
                                                }
                                                this->front.send_data_indication_ex(
                                                    mcs.channelId,
                                                    copy_stream.get_data(),
                                                    copy_stream.get_offset()
                                                );

                                                next_packet = sec.payload.get_data_end();

                                                break;
                                            }

                                            {
                                                ShareData_Recv sdata(sctrl.payload, &this->mppc_dec);
                                                switch (sdata.pdutype2) {
                                                case PDUTYPE2_UPDATE:
                                                    {
                                                        if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_UPDATE"); }
                                                        // MS-RDPBCGR: 1.3.6
                                                        // -----------------
                                                        // The most fundamental output that a server can send to a connected client
                                                        // is bitmap images of the remote session using the Update Bitmap PDU. This
                                                        // allows the client to render the working space and enables a user to
                                                        // interact with the session running on the server. The global palette
                                                        // information for a session is sent to the client in the Update Palette PDU.

                                                        SlowPath::GraphicsUpdate_Recv gur(sdata.payload);
                                                        switch (gur.update_type) {
                                                        case RDP_UPDATE_ORDERS:
                                                            if (this->verbose & 8){ LOG(LOG_INFO, "RDP_UPDATE_ORDERS"); }
                                                            this->front.begin_update();
                                                            this->orders.process_orders(this->bpp, sdata.payload, false, *this->gd,
                                                                                        this->front_width, this->front_height);
                                                            this->front.end_update();
                                                            break;
                                                        case RDP_UPDATE_BITMAP:
                                                            if (this->verbose & 8){ LOG(LOG_INFO, "RDP_UPDATE_BITMAP");}
                                                            this->front.begin_update();
                                                            this->process_bitmap_updates(sdata.payload, false);
                                                            this->front.end_update();
                                                            break;
                                                        case RDP_UPDATE_PALETTE:
                                                            if (this->verbose & 8){ LOG(LOG_INFO, "RDP_UPDATE_PALETTE");}
                                                            this->front.begin_update();
                                                            this->process_palette(sdata.payload, false);
                                                            this->front.end_update();
                                                            break;
                                                        case RDP_UPDATE_SYNCHRONIZE:
                                                            if (this->verbose & 8){ LOG(LOG_INFO, "RDP_UPDATE_SYNCHRONIZE");}
                                                            sdata.payload.in_skip_bytes(2);
                                                            break;
                                                        default:
                                                            if (this->verbose & 8){ LOG(LOG_WARNING, "mod_rdp::MOD_RDP_CONNECTED:RDP_UPDATE_UNKNOWN");}
                                                            break;
                                                        }
                                                    }
                                                    break;
                                                case PDUTYPE2_CONTROL:
                                                    if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_CONTROL");}
                                                    TODO("CGR: Data should actually be consumed");
                                                        sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                                    break;
                                                case PDUTYPE2_SYNCHRONIZE:
                                                    if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_SYNCHRONIZE");}
                                                    TODO("CGR: Data should actually be consumed");
                                                        sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                                    break;
                                                case PDUTYPE2_POINTER:
                                                    if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_POINTER");}
                                                    this->process_pointer_pdu(sdata.payload, this);
                                                    TODO("CGR: Data should actually be consumed");
                                                        sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                                    break;
                                                case PDUTYPE2_PLAY_SOUND:
                                                    if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_PLAY_SOUND");}
                                                    TODO("CGR: Data should actually be consumed");
                                                        sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                                    break;
                                                case PDUTYPE2_SAVE_SESSION_INFO:
                                                    if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_SAVE_SESSION_INFO");}
                                                    TODO("CGR: Data should actually be consumed");
                                                    this->process_save_session_info(sdata.payload);
                                                    break;
                                                case PDUTYPE2_SET_ERROR_INFO_PDU:
                                                    if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_SET_ERROR_INFO_PDU");}
                                                    this->process_disconnect_pdu(sdata.payload);
                                                    break;
                                                case PDUTYPE2_SHUTDOWN_DENIED:
                                                    //if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_SHUTDOWN_DENIED");}
                                                    LOG(LOG_INFO, "PDUTYPE2_SHUTDOWN_DENIED Received");
                                                    break;

                                                case PDUTYPE2_SET_KEYBOARD_INDICATORS:
                                                    {
                                                        if (this->verbose & 8){ LOG(LOG_INFO, "PDUTYPE2_SET_KEYBOARD_INDICATORS");}

                                                        sdata.payload.in_skip_bytes(2); // UnitId(2)

                                                        uint16_t LedFlags = sdata.payload.in_uint16_le();

                                                        this->front.set_keyboard_indicators(LedFlags);

                                                        REDASSERT(sdata.payload.get_current() == sdata.payload.get_data_end());
                                                    }
                                                    break;

                                                default:
                                                    LOG(LOG_WARNING, "PDUTYPE2 unsupported tag=%u", sdata.pdutype2);
                                                    TODO("CGR: Data should actually be consumed");
                                                    sdata.payload.in_skip_bytes(sdata.payload.in_remain());
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                        break;
                                    case PDUTYPE_DEMANDACTIVEPDU:
                                        {
                                            if (this->verbose & 128){
                                                 LOG(LOG_INFO, "PDUTYPE_DEMANDACTIVEPDU");
                                            }

                                            this->orders.reset();

        // 2.2.1.13.1.1 Demand Active PDU Data (TS_DEMAND_ACTIVE_PDU)
        // ==========================================================

        //    shareControlHeader (6 bytes): Share Control Header (section 2.2.8.1.1.1.1 ) containing information
        //  about the packet. The type subfield of the pduType field of the Share Control Header MUST be set to
        // PDUTYPE_DEMANDACTIVEPDU (1).

        //    shareId (4 bytes): A 32-bit, unsigned integer. The share identifier for the packet (see [T128]
        // section 8.4.2 for more information regarding share IDs).

                                            this->share_id = sctrl.payload.in_uint32_le();

        //    lengthSourceDescriptor (2 bytes): A 16-bit, unsigned integer. The size in bytes of the sourceDescriptor
        // field.
                                            uint16_t lengthSourceDescriptor = sctrl.payload.in_uint16_le();

        //    lengthCombinedCapabilities (2 bytes): A 16-bit, unsigned integer. The combined size in bytes of the
        // numberCapabilities, pad2Octets, and capabilitySets fields.

                                            uint16_t lengthCombinedCapabilities = sctrl.payload.in_uint16_le();

        //    sourceDescriptor (variable): A variable-length array of bytes containing a source descriptor (see
        // [T128] section 8.4.1 for more information regarding source descriptors).

                                            TODO("before skipping we should check we do not go outside current stream");
                                            sctrl.payload.in_skip_bytes(lengthSourceDescriptor);

        // numberCapabilities (2 bytes): A 16-bit, unsigned integer. The number of capability sets included in the
        // Demand Active PDU.

        // pad2Octets (2 bytes): A 16-bit, unsigned integer. Padding. Values in this field MUST be ignored.

        // capabilitySets (variable): An array of Capability Set (section 2.2.1.13.1.1.1) structures. The number
        //  of capability sets is specified by the numberCapabilities field.

                                            this->process_server_caps(sctrl.payload, lengthCombinedCapabilities);

        // sessionId (4 bytes): A 32-bit, unsigned integer. The session identifier. This field is ignored by the client.

                                            uint32_t sessionId = sctrl.payload.in_uint32_le();
                                            (void)sessionId;

                                            this->send_confirm_active();
                                            this->send_synchronise();
                                            this->send_control(RDP_CTL_COOPERATE);
                                            this->send_control(RDP_CTL_REQUEST_CONTROL);

                                            /* Including RDP 5.0 capabilities */
                                            if (this->use_rdp5){
                                                LOG(LOG_INFO, "use rdp5");
                                                if (this->enable_persistent_disk_bitmap_cache &&
                                                    this->persist_bitmap_cache_on_disk) {
                                                    if (!this->deactivation_reactivation_in_progress) {
                                                        this->send_persistent_key_list();
                                                    }
                                                }
                                                this->send_fonts(3);
                                            }
                                            else{
                                                LOG(LOG_INFO, "not using rdp5");
                                                this->send_fonts(1);
                                                this->send_fonts(2);
                                            }

                                            this->send_input(0, RDP_INPUT_SYNCHRONIZE, 0, 0, 0);

                                            LOG(LOG_INFO, "Resizing to %ux%ux%u", this->front_width, this->front_height, this->bpp);
                                            if (this->transparent_recorder) {
                                                this->transparent_recorder->server_resize(this->front_width,
                                                    this->front_height, this->bpp);
                                            }
                                            if (-1 == this->front.server_resize(this->front_width, this->front_height, this->bpp)){
                                                LOG(LOG_ERR, "Resize not available on older clients,"
                                                    " change client resolution to match server resolution");
                                                throw Error(ERR_RDP_RESIZE_NOT_AVAILABLE);
                                            }
//                                            this->orders.reset();
                                            this->connection_finalization_state = WAITING_SYNCHRONIZE;

//                                            this->deactivation_reactivation_in_progress = false;
                                        }
                                        break;
                                    case PDUTYPE_DEACTIVATEALLPDU:
                                        if (this->verbose & 128){ LOG(LOG_INFO, "PDUTYPE_DEACTIVATEALLPDU"); }
                                        LOG(LOG_INFO, "Deactivate All PDU");
                                        this->deactivation_reactivation_in_progress = true;
                                        TODO("CGR: Data should actually be consumed");
                                            TODO("CGR: Check we are indeed expecting Synchronize... dubious");
                                            this->connection_finalization_state = WAITING_SYNCHRONIZE;
                                        break;
                                    case PDUTYPE_SERVER_REDIR_PKT:
                                        {
                                            if (this->verbose & 128){
                                                LOG(LOG_INFO, "PDUTYPE_SERVER_REDIR_PKT");
                                            }
                                            sctrl.payload.in_skip_bytes(2);
                                            ServerRedirectionPDU server_redirect;
                                            server_redirect.receive(sctrl.payload);
                                            sctrl.payload.in_skip_bytes(1);
                                            server_redirect.export_to_redirection_info(this->redir_info);
                                            if (this->verbose & 128){
                                                server_redirect.log(LOG_INFO, "Got Packet");
                                                this->redir_info.log(LOG_INFO, "RInfo Ini");
                                            }
                                            if (!server_redirect.Noredirect()) {
                                                LOG(LOG_ERR, "Server Redirection thrown");
                                                throw Error(ERR_RDP_SERVER_REDIR);
                                            }
                                        }
                                        break;
                                    default:
                                        LOG(LOG_INFO, "unknown PDU %u", sctrl.pduType);
                                        break;
                                    }
                                TODO("check sctrl.payload is completely consumed");
                                }
                            }
                        }
                    }
                }
            }
            catch(Error const & e){
                if (e.id == ERR_RDP_SERVER_REDIR) {
                    throw;
                }
                if (this->acl &&
                    (e.id != ERR_MCS_APPID_IS_MCS_DPUM))
                {
                    char message[128];
                    snprintf(message, sizeof(message), "Code=%d", e.id);
                    this->acl->report("SESSION_EXCEPTION", message);

                    this->end_session_reason.clear();
                    this->end_session_message.clear();
                }

                StaticOutStream<256> stream;
                X224::DR_TPDU_Send x224(stream, X224::REASON_NOT_SPECIFIED);
                try {
                    this->nego.trans.send(stream.get_data(), stream.get_offset());
                    LOG(LOG_INFO, "Connection to server closed");
                }
                catch(Error const & e){
                    LOG(LOG_INFO, "Connection to server Already closed: error=%d", e.id);
                };

                this->event.signal = BACK_EVENT_NEXT;

                if (this->enable_session_probe && this->enable_session_probe_loading_mask) {
                    this->front.disable_input_event_and_graphics_update(false);
                }

                if ((e.id == ERR_TRANSPORT_TLS_CERTIFICATE_CHANGED) ||
                    (e.id == ERR_TRANSPORT_TLS_CERTIFICATE_MISSED) ||
                    (e.id == ERR_TRANSPORT_TLS_CERTIFICATE_CORRUPTED) ||
                    (e.id == ERR_TRANSPORT_TLS_CERTIFICATE_INACCESSIBLE) ||
                    (e.id == ERR_NLA_AUTHENTICATION_FAILED))
                {
                    throw;
                }
            }
        }

        if (this->open_session_timeout) {
            switch(this->open_session_timeout_checker.check(now)) {
            case TimeoutT<time_t>::TIMEOUT_REACHED:
                if (this->error_message) {
                    *this->error_message = "Logon timer expired!";
                }

                if (this->acl)
                {
                    this->acl->report("CONNECTION_FAILED", "Logon timer expired.");
                }

                if (this->enable_session_probe && this->enable_session_probe_loading_mask) {
                    this->front.disable_input_event_and_graphics_update(false);
                }

                LOG(LOG_ERR,
                    "Logon timer expired on %s. The session will be disconnected.",
                    this->hostname);
                throw Error(ERR_RDP_OPEN_SESSION_TIMEOUT);
            break;
            case TimeoutT<time_t>::TIMEOUT_NOT_REACHED:
                this->event.set(1000000);
            break;
            case TimeoutT<time_t>::TIMEOUT_INACTIVE:
            break;
            }
        }

        if (this->session_probe_event.set_state && this->session_probe_event.waked_up_by_time) {
            REDASSERT(this->enable_session_probe);

            this->session_probe_event.reset();
            this->session_probe_event.waked_up_by_time = false;

            if (this->session_probe_launch_timeout && !this->session_probe_is_ready) {
                LOG((this->session_probe_on_launch_failure_disconnect_user ? LOG_ERR : LOG_WARNING), "Session Probe is not ready yet!");

                const bool need_full_screen_update =
                    (this->enable_session_probe_loading_mask ?
                     this->front.disable_input_event_and_graphics_update(false) :
                     false);

                if (!this->session_probe_on_launch_failure_disconnect_user) {
                    if (need_full_screen_update) {
                        LOG(LOG_INFO, "Force full screen update. Rect=(0, 0, %u, %u)",
                            this->front_width, this->front_height);
                        this->rdp_input_invalidate(Rect(0, 0, this->front_width, this->front_height));
                    }
                }
                else {
                    throw Error(ERR_SESSION_PROBE_LAUNCH);
                }
            }

            if (this->session_probe_is_ready && this->session_probe_keepalive_timeout) {
                if (!this->session_probe_keep_alive_received) {
                    if (this->enable_session_probe_loading_mask) {
                        this->front.disable_input_event_and_graphics_update(false);
                    }

                    LOG(LOG_ERR, "No keep alive received from Session Probe!");

                    if (this->session_probe_close_pending) {
                        throw Error(ERR_SESSION_PROBE_CLOSE_PENDING);
                    }

                    throw Error(ERR_SESSION_PROBE_KEEPALIVE);
                }
                else {
                    if (this->verbose & 0x10000) {
                        LOG(LOG_INFO, "Session Probe keep alive requested");
                    }
                    this->session_probe_keep_alive_received = false;

                    StaticOutStream<1024> out_s;

                    const size_t message_length_offset = out_s.get_offset();
                    out_s.out_clear_bytes(sizeof(uint16_t));

                    {
                        char string[] = "Request=Keep-Alive";
                        out_s.out_copy_bytes(string, sizeof(string)-1u);
                    }
                    out_s.out_clear_bytes(1);   // Null character

                    out_s.set_out_uint16_le(
                        out_s.get_offset() - message_length_offset - sizeof(uint16_t),
                        message_length_offset);

                    InStream in_s(out_s.get_data(), out_s.get_offset());
                    this->send_to_mod_channel(
                        "sespro", in_s, in_s.get_capacity(),
                        CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST
                    );

                    this->session_probe_event.set(this->session_probe_keepalive_timeout * 1000);
                }
            }
        }
    }   // draw_event

    wait_obj * get_secondary_event() override {
        if (this->session_probe_event.set_state) {
            return &this->session_probe_event;
        }

        return nullptr;
    }

    // 1.3.1.3 Deactivation-Reactivation Sequence
    // ==========================================

    // After the connection sequence has run to completion, the server may determine
    // that the client needs to be connected to a waiting, disconnected session. To
    // accomplish this task the server signals the client with a Deactivate All PDU.
    // A Deactivate All PDU implies that the connection will be dropped or that a
    // capability renegotiation will occur. If a capability renegotiation needs to
    // be performed then the server will re-execute the connection sequence,
    // starting with the Demand Active PDU (the Capability Negotiation and
    // Connection Finalization phases as described in section 1.3.1.1) but excluding
    // the Persistent Key List PDU.


    // 2.2.1.13.1.1 Demand Active PDU Data (TS_DEMAND_ACTIVE_PDU)
    // ==========================================================
    // The TS_DEMAND_ACTIVE_PDU structure is a standard T.128 Demand Active PDU (see [T128] section 8.4.1).

    // shareControlHeader (6 bytes): Share Control Header (section 2.2.8.1.1.1.1) containing information about the packet. The type subfield of the pduType field of the Share Control Header MUST be set to PDUTYPE_DEMANDACTIVEPDU (1).

    // shareId (4 bytes): A 32-bit, unsigned integer. The share identifier for the packet (see [T128] section 8.4.2 for more information regarding share IDs).

    // lengthSourceDescriptor (2 bytes): A 16-bit, unsigned integer. The size in bytes of the sourceDescriptor field.

    // lengthCombinedCapabilities (2 bytes): A 16-bit, unsigned integer. The combined size in bytes of the numberCapabilities, pad2Octets, and capabilitySets fields.

    // sourceDescriptor (variable): A variable-length array of bytes containing a source descriptor (see [T128] section 8.4.1 for more information regarding source descriptors).

    // numberCapabilities (2 bytes): A 16-bit, unsigned integer. The number of capability sets include " in the Demand Active PDU.

    // pad2Octets (2 bytes): A 16-bit, unsigned integer. Padding. Values in this field MUST be ignored.

    // capabilitySets (variable): An array of Capability Set (section 2.2.1.13.1.1.1) structures. The number of capability sets is specified by the numberCapabilities field.

    // sessionId (4 bytes): A 32-bit, unsigned integer. The session identifier. This field is ignored by the client.

    void send_confirm_active() {
        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_confirm_active");
        }

        this->send_data_request_ex(
            GCC::MCS_GLOBAL_CHANNEL,
            [this](StreamSize<65536>, OutStream & stream) {
                RDP::ConfirmActivePDU_Send confirm_active_pdu(stream);

                confirm_active_pdu.emit_begin(this->share_id);

                GeneralCaps general_caps;
                general_caps.extraflags  =
                    this->use_rdp5
                    ? NO_BITMAP_COMPRESSION_HDR | AUTORECONNECT_SUPPORTED | LONG_CREDENTIALS_SUPPORTED
                    : 0
                    ;
                // Slow/Fast-path
                general_caps.extraflags |=
                    this->enable_fastpath_server_update
                    ? FASTPATH_OUTPUT_SUPPORTED
                    : 0
                    ;
                if (this->enable_transparent_mode) {
                    this->front.retrieve_client_capability_set(general_caps);
                }
                if (this->verbose & 1) {
                    general_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(general_caps);

                BitmapCaps bitmap_caps;
                TODO("Client SHOULD set this field to the color depth requested in the Client Core Data")
                bitmap_caps.preferredBitsPerPixel = this->bpp;
                //bitmap_caps.preferredBitsPerPixel = this->front_bpp;
                bitmap_caps.desktopWidth          = this->front_width;
                bitmap_caps.desktopHeight         = this->front_height;
                bitmap_caps.bitmapCompressionFlag = 0x0001; // This field MUST be set to TRUE (0x0001).
                //bitmap_caps.drawingFlags = DRAW_ALLOW_DYNAMIC_COLOR_FIDELITY | DRAW_ALLOW_COLOR_SUBSAMPLING | DRAW_ALLOW_SKIP_ALPHA;
                bitmap_caps.drawingFlags = DRAW_ALLOW_SKIP_ALPHA;
                if (this->enable_transparent_mode) {
                    this->front.retrieve_client_capability_set(bitmap_caps);
                }
                if (this->verbose & 1) {
                    bitmap_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(bitmap_caps);

                OrderCaps order_caps;
                order_caps.numberFonts                                   = 0;
                order_caps.orderFlags                                    = /*0x2a*/
                                                                            NEGOTIATEORDERSUPPORT   /* 0x02 */
                                                                        | ZEROBOUNDSDELTASSUPPORT /* 0x08 */
                                                                        | COLORINDEXSUPPORT       /* 0x20 */
                                                                        | ORDERFLAGS_EXTRA_FLAGS  /* 0x80 */
                                                                        ;
                order_caps.orderSupport[TS_NEG_DSTBLT_INDEX]             = 1;
                order_caps.orderSupport[TS_NEG_MULTIDSTBLT_INDEX]        = (this->enable_multidstblt     ? 1 : 0);
                order_caps.orderSupport[TS_NEG_MULTIOPAQUERECT_INDEX]    = (this->enable_multiopaquerect ? 1 : 0);
                order_caps.orderSupport[TS_NEG_MULTIPATBLT_INDEX]        = (this->enable_multipatblt     ? 1 : 0);
                order_caps.orderSupport[TS_NEG_MULTISCRBLT_INDEX]        = (this->enable_multiscrblt     ? 1 : 0);
                order_caps.orderSupport[TS_NEG_PATBLT_INDEX]             = 1;
                order_caps.orderSupport[TS_NEG_SCRBLT_INDEX]             = 1;
                order_caps.orderSupport[TS_NEG_MEMBLT_INDEX]             = 1;
                order_caps.orderSupport[TS_NEG_MEM3BLT_INDEX]            = (this->enable_mem3blt         ? 1 : 0);
                order_caps.orderSupport[TS_NEG_LINETO_INDEX]             = 1;
                order_caps.orderSupport[TS_NEG_MULTI_DRAWNINEGRID_INDEX] = 0;
                order_caps.orderSupport[UnusedIndex3]                    = 1;
                order_caps.orderSupport[UnusedIndex5]                    = 1;
                order_caps.orderSupport[TS_NEG_POLYGON_SC_INDEX]         = (this->enable_polygonsc       ? 1 : 0);
                order_caps.orderSupport[TS_NEG_POLYGON_CB_INDEX]         = (this->enable_polygoncb       ? 1 : 0);
                order_caps.orderSupport[TS_NEG_POLYLINE_INDEX]           = (this->enable_polyline        ? 1 : 0);
                order_caps.orderSupport[TS_NEG_ELLIPSE_SC_INDEX]         = (this->enable_ellipsesc       ? 1 : 0);
                order_caps.orderSupport[TS_NEG_ELLIPSE_CB_INDEX]         = (this->enable_ellipsecb       ? 1 : 0);
                order_caps.orderSupport[TS_NEG_INDEX_INDEX]              = 1;

                order_caps.textFlags                                     = 0x06a1;
                order_caps.orderSupportExFlags                           = ORDERFLAGS_EX_ALTSEC_FRAME_MARKER_SUPPORT;
                order_caps.textANSICodePage                              = 0x4e4; // Windows-1252 codepage is passed (latin-1)

                // Apparently, these primary drawing orders are supported
                // by both rdesktop and xfreerdp :
                // TS_NEG_DSTBLT_INDEX
                // TS_NEG_PATBLT_INDEX
                // TS_NEG_SCRBLT_INDEX
                // TS_NEG_MEMBLT_INDEX
                // TS_NEG_LINETO_INDEX
                // others orders may not be supported.

                // intersect with client order capabilities
                // which may not be supported by clients.
                this->front.intersect_order_caps(TS_NEG_DSTBLT_INDEX,             order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_PATBLT_INDEX,             order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_SCRBLT_INDEX,             order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_LINETO_INDEX,             order_caps.orderSupport);

                this->front.intersect_order_caps(TS_NEG_MULTIDSTBLT_INDEX,        order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_MULTIOPAQUERECT_INDEX,    order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_MULTIPATBLT_INDEX,        order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_MULTISCRBLT_INDEX,        order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_MEMBLT_INDEX,             order_caps.orderSupport);
                if ((this->verbose & 1) && (!order_caps.orderSupport[TS_NEG_MEMBLT_INDEX])) {
                    LOG(LOG_INFO, "MemBlt Primary Drawing Order is disabled.");
                }
                this->front.intersect_order_caps(TS_NEG_MEM3BLT_INDEX,            order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_MULTI_DRAWNINEGRID_INDEX, order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_POLYGON_SC_INDEX,         order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_POLYGON_CB_INDEX,         order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_POLYLINE_INDEX,           order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_ELLIPSE_SC_INDEX,         order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_ELLIPSE_CB_INDEX,         order_caps.orderSupport);
                this->front.intersect_order_caps(TS_NEG_INDEX_INDEX,              order_caps.orderSupport);

                this->front.intersect_order_caps_ex(order_caps);

                // LOG(LOG_INFO, ">>>>>>>>ORDER CAPABILITIES : ELLIPSE : %d",
                //     order_caps.orderSupport[TS_NEG_ELLIPSE_SC_INDEX]);
                if (this->enable_transparent_mode) {
                    this->front.retrieve_client_capability_set(order_caps);
                }
                if (this->verbose & 1) {
                    order_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(order_caps);


                BmpCacheCaps bmpcache_caps;
                bmpcache_caps.cache0Entries         = 0x258;
                bmpcache_caps.cache0MaximumCellSize = nbbytes(this->bpp) * 0x100;
                bmpcache_caps.cache1Entries         = 0x12c;
                bmpcache_caps.cache1MaximumCellSize = nbbytes(this->bpp) * 0x400;
                bmpcache_caps.cache2Entries         = 0x106;
                bmpcache_caps.cache2MaximumCellSize = nbbytes(this->bpp) * 0x1000;

                BmpCache2Caps bmpcache2_caps;
                bmpcache2_caps.cacheFlags           = PERSISTENT_KEYS_EXPECTED_FLAG | (this->enable_cache_waiting_list ? ALLOW_CACHE_WAITING_LIST_FLAG : 0);
                bmpcache2_caps.numCellCaches        = 3;
                bmpcache2_caps.bitmapCache0CellInfo = 120;
                bmpcache2_caps.bitmapCache1CellInfo = 120;
                bmpcache2_caps.bitmapCache2CellInfo = (2553 | 0x80000000);

                bool use_bitmapcache_rev2 = false;

                if (this->enable_transparent_mode) {
                    if (!this->front.retrieve_client_capability_set(bmpcache_caps)) {
                        this->front.retrieve_client_capability_set(bmpcache2_caps);
                        use_bitmapcache_rev2 = true;
                    }
                }
                else {
                    use_bitmapcache_rev2 = this->enable_persistent_disk_bitmap_cache;
                }

                if (use_bitmapcache_rev2) {
                    if (this->verbose & 1) {
                        bmpcache2_caps.log("Sending to server");
                    }
                    confirm_active_pdu.emit_capability_set(bmpcache2_caps);

                    if (!this->enable_transparent_mode && !this->deactivation_reactivation_in_progress) {
                        this->orders.create_cache_bitmap(this->bpp,
                            120,   nbbytes(this->bpp) * 16 * 16, false,
                            120,   nbbytes(this->bpp) * 32 * 32, false,
                            2553,  nbbytes(this->bpp) * 64 * 64, this->enable_persistent_disk_bitmap_cache,
                            this->cache_verbose);
                    }
                }
                else {
                    if (this->verbose & 1) {
                        bmpcache_caps.log("Sending to server");
                    }
                    confirm_active_pdu.emit_capability_set(bmpcache_caps);

                    if (!this->enable_transparent_mode && !this->deactivation_reactivation_in_progress) {
                        this->orders.create_cache_bitmap(this->bpp,
                            0x258, nbbytes(this->bpp) * 0x100,   false,
                            0x12c, nbbytes(this->bpp) * 0x400,   false,
                            0x106, nbbytes(this->bpp) * 0x1000,  false,
                            this->cache_verbose);
                    }
                }

                ColorCacheCaps colorcache_caps;
                if (this->verbose & 1) {
                    colorcache_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(colorcache_caps);

                ActivationCaps activation_caps;
                if (this->verbose & 1) {
                    activation_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(activation_caps);

                ControlCaps control_caps;
                if (this->verbose & 1) {
                    control_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(control_caps);

                PointerCaps pointer_caps;
                pointer_caps.len                       = 10;
                if (this->enable_new_pointer == false) {
                    pointer_caps.pointerCacheSize      = 0;
                    pointer_caps.colorPointerCacheSize = 20;
                    pointer_caps.len                   = 8;
                    REDASSERT(pointer_caps.colorPointerCacheSize <= sizeof(this->cursors) / sizeof(Pointer));
                }
                if (this->verbose & 1) {
                    pointer_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(pointer_caps);

                ShareCaps share_caps;
                if (this->verbose & 1) {
                    share_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(share_caps);

                InputCaps input_caps;
                if (this->verbose & 1) {
                    input_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(input_caps);

                SoundCaps sound_caps;
                if (this->verbose & 1) {
                    sound_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(sound_caps);

                FontCaps font_caps;
                if (this->verbose & 1) {
                    font_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(font_caps);

                GlyphCacheCaps glyphcache_caps;
                if (this->enable_glyph_cache) {
                    this->front.retrieve_client_capability_set(glyphcache_caps);

                    glyphcache_caps.FragCache         = 0;  // Not yet supported
                    glyphcache_caps.GlyphSupportLevel &= GlyphCacheCaps::GLYPH_SUPPORT_PARTIAL;
                }
                if (this->verbose & 1) {
                    glyphcache_caps.log("Sending to server");
                }
                confirm_active_pdu.emit_capability_set(glyphcache_caps);

                if (this->remote_program) {
                    RailCaps rail_caps;
                    rail_caps.RailSupportLevel = TS_RAIL_LEVEL_SUPPORTED;
                    if (this->verbose & 1) {
                        rail_caps.log("Sending to server");
                    }
                    confirm_active_pdu.emit_capability_set(rail_caps);

                    WindowListCaps window_list_caps;
                    window_list_caps.WndSupportLevel = TS_WINDOW_LEVEL_SUPPORTED;
                    window_list_caps.NumIconCaches = 3;
                    window_list_caps.NumIconCacheEntries = 12;
                    if (this->verbose & 1) {
                        window_list_caps.log("Sending to server");
                    }
                    confirm_active_pdu.emit_capability_set(window_list_caps);
                }
                confirm_active_pdu.emit_end();
            },
            [this](StreamSize<256>, OutStream & sctrl_header, std::size_t packet_size) {
                // shareControlHeader (6 bytes): Share Control Header (section 2.2.8.1.1.1.1)
                // containing information about the packet. The type subfield of the pduType
                // field of the Share Control Header MUST be set to PDUTYPE_DEMANDACTIVEPDU (1).
                ShareControl_Send(sctrl_header, PDUTYPE_CONFIRMACTIVEPDU,
                    this->userid + GCC::MCS_USERCHANNEL_BASE, packet_size);
            }
        );

        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_confirm_active done");
            LOG(LOG_INFO, "Waiting for answer to confirm active");
        }
    }   // send_confirm_active

    void process_pointer_pdu(InStream & stream, mod_api * mod)
    {
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::process_pointer_pdu");
        }

        int message_type = stream.in_uint16_le();
        stream.in_skip_bytes(2); /* pad */
        switch (message_type) {
        case RDP_POINTER_CACHED:
            if (this->verbose & 4){
                LOG(LOG_INFO, "Process pointer cached");
            }
            this->process_cached_pointer_pdu(stream);
            if (this->verbose & 4){
                LOG(LOG_INFO, "Process pointer cached done");
            }
            break;
        case RDP_POINTER_COLOR:
            if (this->verbose & 4){
                LOG(LOG_INFO, "Process pointer color");
            }
            this->process_system_pointer_pdu(stream);
            if (this->verbose & 4){
                LOG(LOG_INFO, "Process pointer system done");
            }
            break;
        case RDP_POINTER_NEW:
            if (this->verbose & 4){
                LOG(LOG_INFO, "Process pointer new");
            }
            if (enable_new_pointer) {
                this->process_new_pointer_pdu(stream); // Pointer with arbitrary color depth
            }
            if (this->verbose & 4){
                LOG(LOG_INFO, "Process pointer new done");
            }
            break;
        case RDP_POINTER_SYSTEM:
            if (this->verbose & 4){
                LOG(LOG_INFO, "Process pointer system");
            }
        case RDP_POINTER_MOVE:
            {
                if (this->verbose & 4) {
                    LOG(LOG_INFO, "Process pointer move");
                }
                uint16_t xPos = stream.in_uint16_le();
                uint16_t yPos = stream.in_uint16_le();
                this->front.update_pointer_position(xPos, yPos);
            }
            break;
        default:
            break;
        }
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::process_pointer_pdu done");
        }
    }

    void process_palette(InStream & stream, bool fast_path) {
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::process_palette");
        }

        RDP::UpdatePaletteData_Recv(stream, fast_path, this->orders.global_palette);
        this->front.set_mod_palette(this->orders.global_palette);

        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::process_palette done");
        }
    }

    // 2.2.5.1.1 Set Error Info PDU Data (TS_SET_ERROR_INFO_PDU)
    // =========================================================
    // The TS_SET_ERROR_INFO_PDU structure contains the contents of the Set Error
    // Info PDU, which is a Share Data Header (section 2.2.8.1.1.1.2) with an error
    // value field.

    // shareDataHeader (18 bytes): Share Data Header containing information about
    // the packet. The type subfield of the pduType field of the Share Control
    // Header (section 2.2.8.1.1.1.1) MUST be set to PDUTYPE_DATAPDU (7). The
    // pduType2 field of the Share Data Header MUST be set to
    // PDUTYPE2_SET_ERROR_INFO_PDU (47), and the pduSource field MUST be set to 0.

    // errorInfo (4 bytes): A 32-bit, unsigned integer. Error code.

    // Protocol-independent codes:
    // +---------------------------------------------+-----------------------------+
    // | 0x00000001 ERRINFO_RPC_INITIATED_DISCONNECT | The disconnection was       |
    // |                                             | initiated by an             |
    // |                                             | administrative tool on the  |
    // |                                             | server in another session.  |
    // +---------------------------------------------+-----------------------------+
    // | 0x00000002 ERRINFO_RPC_INITIATED_LOGOFF     | The disconnection was due   |
    // |                                             | to a forced logoff initiated|
    // |                                             | by an administrative tool   |
    // |                                             | on the server in another    |
    // |                                             | session.                    |
    // +---------------------------------------------+-----------------------------+
    // | 0x00000003 ERRINFO_IDLE_TIMEOUT             | The idle session limit timer|
    // |                                             | on the server has elapsed.  |
    // +---------------------------------------------+-----------------------------+
    // | 0x00000004 ERRINFO_LOGON_TIMEOUT            | The active session limit    |
    // |                                             | timer on the server has     |
    // |                                             | elapsed.                    |
    // +---------------------------------------------+-----------------------------+
    // | 0x00000005                                  | Another user connected to   |
    // | ERRINFO_DISCONNECTED_BY_OTHERCONNECTION     | the server, forcing the     |
    // |                                             | disconnection of the current|
    // |                                             | connection.                 |
    // +---------------------------------------------+-----------------------------+
    // | 0x00000006 ERRINFO_OUT_OF_MEMORY            | The server ran out of       |
    // |                                             | available memory resources. |
    // +---------------------------------------------+-----------------------------+
    // | 0x00000007 ERRINFO_SERVER_DENIED_CONNECTION | The server denied the       |
    // |                                             | connection.                 |
    // +---------------------------------------------+-----+-----------------------+
    // | 0x00000009                                  | The user cannot connect to  |
    // | ERRINFO_SERVER_INSUFFICIENT_PRIVILEGES      | the server due to           |
    // |                                             | insufficient access         |
    // |                                             | privileges.                 |
    // +---------------------------------------------+-----------------------------+
    // | 0x0000000A                                  | The server does not accept  |
    // | ERRINFO_SERVER_FRESH_CREDENTIALS_REQUIRED   | saved user credentials and  |
    // |                                             | requires that the user enter|
    // |                                             | their credentials for each  |
    // |                                             | connection.                 |
    // +-----------------------------------------+---+-----------------------------+
    // | 0x0000000B                              | The disconnection was initiated |
    // | ERRINFO_RPC_INITIATED_DISCONNECT_BYUSER | by an administrative tool on    |
    // |                                         | the server running in the user's|
    // |                                         | session.                        |
    // +-----------------------------------------+---------------------------------+
    // | 0x0000000C ERRINFO_LOGOFF_BY_USER       | The disconnection was initiated |
    // |                                         | by the user logging off his or  |
    // |                                         | her session on the server.      |
    // +-----------------------------------------+---------------------------------+

    // Protocol-independent licensing codes:
    // +-------------------------------------------+-------------------------------+
    // | 0x00000100 ERRINFO_LICENSE_INTERNAL       | An internal error has occurred|
    // |                                           | in the Terminal Services      |
    // |                                           | licensing component.          |
    // +-------------------------------------------+-------------------------------+
    // | 0x00000101                                | A Remote Desktop License      |
    // | ERRINFO_LICENSE_NO_LICENSE_SERVER         | Server ([MS-RDPELE] section   |
    // |                                           | 1.1) could not be found to    |
    // |                                           | provide a license.            |
    // +-------------------------------------------+-------------------------------+
    // | 0x00000102 ERRINFO_LICENSE_NO_LICENSE     | There are no Client Access    |
    // |                                           | Licenses ([MS-RDPELE] section |
    // |                                           | 1.1) available for the target |
    // |                                           | remote computer.              |
    // +-------------------------------------------+-------------------------------+
    // | 0x00000103 ERRINFO_LICENSE_BAD_CLIENT_MSG | The remote computer received  |
    // |                                           | an invalid licensing message  |
    // |                                           | from the client.              |
    // +-------------------------------------------+-------------------------------+
    // | 0x00000104                                | The Client Access License     |
    // | ERRINFO_LICENSE_HWID_DOESNT_MATCH_LICENSE | ([MS-RDPELE] section 1.1)     |
    // |                                           | stored by the client has been |
    // |                                           |  modified.                    |
    // +-------------------------------------------+-------------------------------+
    // | 0x00000105                                | The Client Access License     |
    // | ERRINFO_LICENSE_BAD_CLIENT_LICENSE        | ([MS-RDPELE] section 1.1)     |
    // |                                           | stored by the client is in an |
    // |                                           | invalid format.               |
    // +-------------------------------------------+-------------------------------+
    // | 0x00000106                                | Network problems have caused  |
    // | ERRINFO_LICENSE_CANT_FINISH_PROTOCOL      | the licensing protocol        |
    // |                                           | ([MS-RDPELE] section 1.3.3)   |
    // |                                           | to be terminated.             |
    // +-------------------------------------------+-------------------------------+
    // | 0x00000107                                | The client prematurely ended  |
    // | ERRINFO_LICENSE_CLIENT_ENDED_PROTOCOL     | the licensing protocol        |
    // |                                           | ([MS-RDPELE] section 1.3.3).  |
    // +---------------------------------------+---+-------------------------------+
    // | 0x00000108                            | A licensing message ([MS-RDPELE]  |
    // | ERRINFO_LICENSE_BAD_CLIENT_ENCRYPTION | sections 2.2 and 5.1) was         |
    // |                                       | incorrectly encrypted.            |
    // +---------------------------------------+-----------------------------------+
    // | 0x00000109                            | The Client Access License         |
    // | ERRINFO_LICENSE_CANT_UPGRADE_LICENSE  | ([MS-RDPELE] section 1.1) stored  |
    // |                                       | by the client could not be        |
    // |                                       | upgraded or renewed.              |
    // +---------------------------------------+-----------------------------------+
    // | 0x0000010A                            | The remote computer is not        |
    // | ERRINFO_LICENSE_NO_REMOTE_CONNECTIONS | licensed to accept remote         |
    // |                                       |  connections.                     |
    // +---------------------------------------+-----------------------------------+

    // Protocol-independent codes generated by Connection Broker:
    // +----------------------------------------------+----------------------------+
    // | Value                                        | Meaning                    |
    // +----------------------------------------------+----------------------------+
    // | 0x0000400                                    | The target endpoint could  |
    // | ERRINFO_CB_DESTINATION_NOT_FOUND             | not be found.              |
    // +----------------------------------------------+----------------------------+
    // | 0x0000402                                    | The target endpoint to     |
    // | ERRINFO_CB_LOADING_DESTINATION               | which the client is being  |
    // |                                              | redirected is              |
    // |                                              | disconnecting from the     |
    // |                                              | Connection Broker.         |
    // +----------------------------------------------+----------------------------+
    // | 0x0000404                                    | An error occurred while    |
    // | ERRINFO_CB_REDIRECTING_TO_DESTINATION        | the connection was being   |
    // |                                              | redirected to the target   |
    // |                                              | endpoint.                  |
    // +----------------------------------------------+----------------------------+
    // | 0x0000405                                    | An error occurred while    |
    // | ERRINFO_CB_SESSION_ONLINE_VM_WAKE            | the target endpoint (a     |
    // |                                              | virtual machine) was being |
    // |                                              | awakened.                  |
    // +----------------------------------------------+----------------------------+
    // | 0x0000406                                    | An error occurred while    |
    // | ERRINFO_CB_SESSION_ONLINE_VM_BOOT            | the target endpoint (a     |
    // |                                              | virtual machine) was being |
    // |                                              | started.                   |
    // +----------------------------------------------+----------------------------+
    // | 0x0000407                                    | The IP address of the      |
    // | ERRINFO_CB_SESSION_ONLINE_VM_NO_DNS          | target endpoint (a virtual |
    // |                                              | machine) cannot be         |
    // |                                              | determined.                |
    // +----------------------------------------------+----------------------------+
    // | 0x0000408                                    | There are no available     |
    // | ERRINFO_CB_DESTINATION_POOL_NOT_FREE         | endpoints in the pool      |
    // |                                              | managed by the Connection  |
    // |                                              | Broker.                    |
    // +----------------------------------------------+----------------------------+
    // | 0x0000409                                    | Processing of the          |
    // | ERRINFO_CB_CONNECTION_CANCELLED              | connection has been        |
    // |                                              | cancelled.                 |
    // +----------------------------------------------+----------------------------+
    // | 0x0000410                                    | The settings contained in  |
    // | ERRINFO_CB_CONNECTION_ERROR_INVALID_SETTINGS | the routingToken field of  |
    // |                                              | the X.224 Connection       |
    // |                                              | Request PDU (section       |
    // |                                              | 2.2.1.1) cannot be         |
    // |                                              | validated.                 |
    // +----------------------------------------------+----------------------------+
    // | 0x0000411                                    | A time-out occurred while  |
    // | ERRINFO_CB_SESSION_ONLINE_VM_BOOT_TIMEOUT    | the target endpoint (a     |
    // |                                              | virtual machine) was being |
    // |                                              | started.                   |
    // +----------------------------------------------+----------------------------+
    // | 0x0000412                                    | A session monitoring error |
    // | ERRINFO_CB_SESSION_ONLINE_VM_SESSMON_FAILED  | occurred while the target  |
    // |                                              | endpoint (a virtual        |
    // |                                              | machine) was being         |
    // |                                              | started.                   |
    // +----------------------------------------------+----------------------------+

    // RDP specific codes:
    // +------------------------------------+--------------------------------------+
    // | 0x000010C9 ERRINFO_UNKNOWNPDUTYPE2 | Unknown pduType2 field in a received |
    // |                                    | Share Data Header (section           |
    // |                                    | 2.2.8.1.1.1.2).                      |
    // +------------------------------------+--------------------------------------+
    // | 0x000010CA ERRINFO_UNKNOWNPDUTYPE  | Unknown pduType field in a received  |
    // |                                    | Share Control Header (section        |
    // |                                    | 2.2.8.1.1.1.1).                      |
    // +------------------------------------+--------------------------------------+
    // | 0x000010CB ERRINFO_DATAPDUSEQUENCE | An out-of-sequence Slow-Path Data PDU|
    // |                                    | (section 2.2.8.1.1.1.1) has been     |
    // |                                    | received.                            |
    // +------------------------------------+--------------------------------------+
    // | 0x000010CD                         | An out-of-sequence Slow-Path Non-Data|
    // | ERRINFO_CONTROLPDUSEQUENCE         | PDU (section 2.2.8.1.1.1.1) has been |
    // |                                    | received.                            |
    // +------------------------------------+--------------------------------------+
    // | 0x000010CE                         | A Control PDU (sections 2.2.1.15 and |
    // | ERRINFO_INVALIDCONTROLPDUACTION    | 2.2.1.16) has been received with an  |
    // |                                    | invalid action field.                |
    // +------------------------------------+--------------------------------------+
    // | 0x000010CF                         | (a) A Slow-Path Input Event (section |
    // | ERRINFO_INVALIDINPUTPDUTYPE        | 2.2.8.1.1.3.1.1) has been received   |
    // |                                    | with an invalid messageType field.   |
    // |                                    | (b) A Fast-Path Input Event (section |
    // |                                    | 2.2.8.1.2.2) has been received with  |
    // |                                    | an invalid eventCode field.          |
    // +------------------------------------+--------------------------------------+
    // | 0x000010D0                         | (a) A Slow-Path Mouse Event (section |
    // | ERRINFO_INVALIDINPUTPDUMOUSE       | 2.2.8.1.1.3.1.1.3) or Extended Mouse |
    // |                                    | Event (section 2.2.8.1.1.3.1.1.4)    |
    // |                                    | has been received with an invalid    |
    // |                                    | pointerFlags field.                  |
    // |                                    | (b) A Fast-Path Mouse Event (section |
    // |                                    | 2.2.8.1.2.2.3) or Fast-Path Extended |
    // |                                    | Mouse Event (section 2.2.8.1.2.2.4)  |
    // |                                    | has been received with an invalid    |
    // |                                    | pointerFlags field.                  |
    // +------------------------------------+--------------------------------------+
    // | 0x000010D1                         | An invalid Refresh Rect PDU (section |
    // | ERRINFO_INVALIDREFRESHRECTPDU      | 2.2.11.2) has been received.         |
    // +------------------------------------+--------------------------------------+
    // | 0x000010D2                         | The server failed to construct the   |
    // | ERRINFO_CREATEUSERDATAFAILED       | GCC Conference Create Response user  |
    // |                                    | data (section 2.2.1.4).              |
    // +------------------------------------+--------------------------------------+
    // | 0x000010D3 ERRINFO_CONNECTFAILED   | Processing during the Channel        |
    // |                                    | Connection phase of the RDP          |
    // |                                    | Connection Sequence (see section     |
    // |                                    | 1.3.1.1 for an overview of the RDP   |
    // |                                    | Connection Sequence phases) has      |
    // |                                    | failed.                              |
    // +------------------------------------+--------------------------------------+
    // | 0x000010D4                         | A Confirm Active PDU (section        |
    // | ERRINFO_CONFIRMACTIVEWRONGSHAREID  | 2.2.1.13.2) was received from the    |
    // |                                    | client with an invalid shareId field.|
    // +------------------------------------+-+------------------------------------+
    // | 0x000010D5                           | A Confirm Active PDU (section      |
    // | ERRINFO_CONFIRMACTIVEWRONGORIGINATOR | 2.2.1.13.2) was received from the  |
    // |                                      | client with an invalid originatorId|
    // |                                      | field.                             |
    // +--------------------------------------+------------------------------------+
    // | 0x000010DA                           | There is not enough data to process|
    // | ERRINFO_PERSISTENTKEYPDUBADLENGTH    | a Persistent Key List PDU (section |
    // |                                      | 2.2.1.17).                         |
    // +--------------------------------------+------------------------------------+
    // | 0x000010DB                           | A Persistent Key List PDU (section |
    // | ERRINFO_PERSISTENTKEYPDUILLEGALFIRST | 2.2.1.17) marked as                |
    // |                                      | PERSIST_PDU_FIRST (0x01) was       |
    // |                                      | received after the reception of a  |
    // |                                      | prior Persistent Key List PDU also |
    // |                                      | marked as PERSIST_PDU_FIRST.       |
    // +--------------------------------------+---+--------------------------------+
    // | 0x000010DC                               | A Persistent Key List PDU      |
    // | ERRINFO_PERSISTENTKEYPDUTOOMANYTOTALKEYS | (section 2.2.1.17) was received|
    // |                                          | which specified a total number |
    // |                                          | of bitmap cache entries larger |
    // |                                          | than 262144.                   |
    // +------------------------------------------+--------------------------------+
    // | 0x000010DD                               | A Persistent Key List PDU      |
    // | ERRINFO_PERSISTENTKEYPDUTOOMANYCACHEKEYS | (section 2.2.1.17) was received|
    // |                                          | which specified an invalid     |
    // |                                          | total number of keys for a     |
    // |                                          | bitmap cache (the number of    |
    // |                                          | entries that can be stored     |
    // |                                          | within each bitmap cache is    |
    // |                                          | specified in the Revision 1 or |
    // |                                          | 2 Bitmap Cache Capability Set  |
    // |                                          | (section 2.2.7.1.4) that is    |
    // |                                          | sent from client to server).   |
    // +------------------------------------------+--------------------------------+
    // | 0x000010DE ERRINFO_INPUTPDUBADLENGTH     | There is not enough data to    |
    // |                                          | process Input Event PDU Data   |
    // |                                          | (section 2.2.8.1.1.3.          |
    // |                                          | 2.2.8.1.2).                    |
    // +------------------------------------------+--------------------------------+
    // | 0x000010DF                               | There is not enough data to    |
    // | ERRINFO_BITMAPCACHEERRORPDUBADLENGTH     | process the shareDataHeader,   |
    // |                                          | NumInfoBlocks, Pad1, and Pad2  |
    // |                                          | fields of the Bitmap Cache     |
    // |                                          | Error PDU Data ([MS-RDPEGDI]   |
    // |                                          | section 2.2.2.3.1.1).          |
    // +------------------------------------------+--------------------------------+
    // | 0x000010E0  ERRINFO_SECURITYDATATOOSHORT | (a) The dataSignature field of |
    // |                                          | the Fast-Path Input Event PDU  |
    // |                                          | (section 2.2.8.1.2) does not   |
    // |                                          | contain enough data.           |
    // |                                          | (b) The fipsInformation and    |
    // |                                          | dataSignature fields of the    |
    // |                                          | Fast-Path Input Event PDU      |
    // |                                          | (section 2.2.8.1.2) do not     |
    // |                                          | contain enough data.           |
    // +------------------------------------------+--------------------------------+
    // | 0x000010E1 ERRINFO_VCHANNELDATATOOSHORT  | (a) There is not enough data   |
    // |                                          | in the Client Network Data     |
    // |                                          | (section 2.2.1.3.4) to read the|
    // |                                          | virtual channel configuration  |
    // |                                          | data.                          |
    // |                                          | (b) There is not enough data   |
    // |                                          | to read a complete Channel     |
    // |                                          | PDU Header (section 2.2.6.1.1).|
    // +------------------------------------------+--------------------------------+
    // | 0x000010E2 ERRINFO_SHAREDATATOOSHORT     | (a) There is not enough data   |
    // |                                          | to process Control PDU Data    |
    // |                                          | (section 2.2.1.15.1).          |
    // |                                          | (b) There is not enough data   |
    // |                                          | to read a complete Share       |
    // |                                          | Control Header (section        |
    // |                                          | 2.2.8.1.1.1.1).                |
    // |                                          | (c) There is not enough data   |
    // |                                          | to read a complete Share Data  |
    // |                                          | Header (section 2.2.8.1.1.1.2) |
    // |                                          | of a Slow-Path Data PDU        |
    // |                                          | (section 2.2.8.1.1.1.1).       |
    // |                                          | (d) There is not enough data   |
    // |                                          | to process Font List PDU Data  |
    // |                                          | (section 2.2.1.18.1).          |
    // +------------------------------------------+--------------------------------+
    // | 0x000010E3 ERRINFO_BADSUPRESSOUTPUTPDU   | (a) There is not enough data   |
    // |                                          | to process Suppress Output PDU |
    // |                                          | Data (section 2.2.11.3.1).     |
    // |                                          | (b) The allowDisplayUpdates    |
    // |                                          | field of the Suppress Output   |
    // |                                          | PDU Data (section 2.2.11.3.1)  |
    // |                                          | is invalid.                    |
    // +------------------------------------------+--------------------------------+
    // | 0x000010E5                               | (a) There is not enough data   |
    // | ERRINFO_CONFIRMACTIVEPDUTOOSHORT         | to read the shareControlHeader,|
    // |                                          | shareId, originatorId,         |
    // |                                          | lengthSourceDescriptor, and    |
    // |                                          | lengthCombinedCapabilities     |
    // |                                          | fields of the Confirm Active   |
    // |                                          | PDU Data (section              |
    // |                                          | 2.2.1.13.2.1).                 |
    // |                                          | (b) There is not enough data   |
    // |                                          | to read the sourceDescriptor,  |
    // |                                          | numberCapabilities, pad2Octets,|
    // |                                          | and capabilitySets fields of   |
    // |                                          | the Confirm Active PDU Data    |
    // |                                          | (section 2.2.1.13.2.1).        |
    // +------------------------------------------+--------------------------------+
    // | 0x000010E7 ERRINFO_CAPABILITYSETTOOSMALL | There is not enough data to    |
    // |                                          | read the capabilitySetType and |
    // |                                          | the lengthCapability fields in |
    // |                                          | a received Capability Set      |
    // |                                          | (section 2.2.1.13.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x000010E8 ERRINFO_CAPABILITYSETTOOLARGE | A Capability Set (section      |
    // |                                          | 2.2.1.13.1.1.1) has been       |
    // |                                          | received with a                |
    // |                                          | lengthCapability field that    |
    // |                                          | contains a value greater than  |
    // |                                          | the total length of the data   |
    // |                                          | received.                      |
    // +------------------------------------------+--------------------------------+
    // | 0x000010E9 ERRINFO_NOCURSORCACHE         | (a) Both the                   |
    // |                                          | colorPointerCacheSize and      |
    // |                                          | pointerCacheSize fields in the |
    // |                                          | Pointer Capability Set         |
    // |                                          | (section 2.2.7.1.5) are set to |
    // |                                          | zero.                          |
    // |                                          | (b) The pointerCacheSize field |
    // |                                          | in the Pointer Capability Set  |
    // |                                          | (section 2.2.7.1.5) is not     |
    // |                                          | present, and the               |
    // |                                          | colorPointerCacheSize field is |
    // |                                          | set to zero.                   |
    // +------------------------------------------+--------------------------------+
    // | 0x000010EA ERRINFO_BADCAPABILITIES       | The capabilities received from |
    // |                                          | the client in the Confirm      |
    // |                                          | Active PDU (section 2.2.1.13.2)|
    // |                                          | were not accepted by the       |
    // |                                          | server.                        |
    // +------------------------------------------+--------------------------------+
    // | 0x000010EC                               | An error occurred while using  |
    // | ERRINFO_VIRTUALCHANNELDECOMPRESSIONERR   | the bulk compressor (section   |
    // |                                          | 3.1.8 and [MS- RDPEGDI] section|
    // |                                          | 3.1.8) to decompress a Virtual |
    // |                                          | Channel PDU (section 2.2.6.1). |
    // +------------------------------------------+--------------------------------+
    // | 0x000010ED                               | An invalid bulk compression    |
    // | ERRINFO_INVALIDVCCOMPRESSIONTYPE         | package was specified in the   |
    // |                                          | flags field of the Channel PDU |
    // |                                          | Header (section 2.2.6.1.1).    |
    // +------------------------------------------+--------------------------------+
    // | 0x000010EF ERRINFO_INVALIDCHANNELID      | An invalid MCS channel ID was  |
    // |                                          | specified in the mcsPdu field  |
    // |                                          | of the Virtual Channel PDU     |
    // |                                          | (section 2.2.6.1).             |
    // +------------------------------------------+--------------------------------+
    // | 0x000010F0 ERRINFO_VCHANNELSTOOMANY      | The client requested more than |
    // |                                          | the maximum allowed 31 static  |
    // |                                          | virtual channels in the Client |
    // |                                          | Network Data (section          |
    // |                                          | 2.2.1.3.4).                    |
    // +------------------------------------------+--------------------------------+
    // | 0x000010F3 ERRINFO_REMOTEAPPSNOTENABLED  | The INFO_RAIL flag (0x00008000)|
    // |                                          | MUST be set in the flags field |
    // |                                          | of the Info Packet (section    |
    // |                                          | 2.2.1.11.1.1) as the session   |
    // |                                          | on the remote server can only  |
    // |                                          | host remote applications.      |
    // +------------------------------------------+--------------------------------+
    // | 0x000010F4 ERRINFO_CACHECAPNOTSET        | The client sent a Persistent   |
    // |                                          | Key List PDU (section 2.2.1.17)|
    // |                                          | without including the          |
    // |                                          | prerequisite Revision 2 Bitmap |
    // |                                          | Cache Capability Set (section  |
    // |                                          | 2.2.7.1.4.2) in the Confirm    |
    // |                                          | Active PDU (section            |
    // |                                          | 2.2.1.13.2).                   |
    // +------------------------------------------+--------------------------------+
    // | 0x000010F5                               | The NumInfoBlocks field in the |
    // |ERRINFO_BITMAPCACHEERRORPDUBADLENGTH2     | Bitmap Cache Error PDU Data is |
    // |                                          | inconsistent with the amount   |
    // |                                          | of data in the Info field      |
    // |                                          | ([MS-RDPEGDI] section          |
    // |                                          | 2.2.2.3.1.1).                  |
    // +------------------------------------------+--------------------------------+
    // | 0x000010F6                               | There is not enough data to    |
    // | ERRINFO_OFFSCRCACHEERRORPDUBADLENGTH     | process an Offscreen Bitmap    |
    // |                                          | Cache Error PDU ([MS-RDPEGDI]  |
    // |                                          | section 2.2.2.3.2).            |
    // +------------------------------------------+--------------------------------+
    // | 0x000010F7                               | There is not enough data to    |
    // | ERRINFO_DNGCACHEERRORPDUBADLENGTH        | process a DrawNineGrid Cache   |
    // |                                          | Error PDU ([MS-RDPEGDI]        |
    // |                                          | section 2.2.2.3.3).            |
    // +------------------------------------------+--------------------------------+
    // | 0x000010F8 ERRINFO_GDIPLUSPDUBADLENGTH   | There is not enough data to    |
    // |                                          | process a GDI+ Error PDU       |
    // |                                          | ([MS-RDPEGDI] section          |
    // |                                          | 2.2.2.3.4).                    |
    // +------------------------------------------+--------------------------------+
    // | 0x00001111 ERRINFO_SECURITYDATATOOSHORT2 | There is not enough data to    |
    // |                                          | read a Basic Security Header   |
    // |                                          | (section 2.2.8.1.1.2.1).       |
    // +------------------------------------------+--------------------------------+
    // | 0x00001112 ERRINFO_SECURITYDATATOOSHORT3 | There is not enough data to    |
    // |                                          | read a Non- FIPS Security      |
    // |                                          | Header (section 2.2.8.1.1.2.2) |
    // |                                          | or FIPS Security Header        |
    // |                                          | (section 2.2.8.1.1.2.3).       |
    // +------------------------------------------+--------------------------------+
    // | 0x00001113 ERRINFO_SECURITYDATATOOSHORT4 | There is not enough data to    |
    // |                                          | read the basicSecurityHeader   |
    // |                                          | and length fields of the       |
    // |                                          | Security Exchange PDU Data     |
    // |                                          | (section 2.2.1.10.1).          |
    // +------------------------------------------+--------------------------------+
    // | 0x00001114 ERRINFO_SECURITYDATATOOSHORT5 | There is not enough data to    |
    // |                                          | read the CodePage, flags,      |
    // |                                          | cbDomain, cbUserName,          |
    // |                                          | cbPassword, cbAlternateShell,  |
    // |                                          | cbWorkingDir, Domain, UserName,|
    // |                                          | Password, AlternateShell, and  |
    // |                                          | WorkingDir fields in the Info  |
    // |                                          | Packet (section 2.2.1.11.1.1). |
    // +------------------------------------------+--------------------------------+
    // | 0x00001115 ERRINFO_SECURITYDATATOOSHORT6 | There is not enough data to    |
    // |                                          | read the CodePage, flags,      |
    // |                                          | cbDomain, cbUserName,          |
    // |                                          | cbPassword, cbAlternateShell,  |
    // |                                          | and cbWorkingDir fields in the |
    // |                                          | Info Packet (section           |
    // |                                          | 2.2.1.11.1.1).                 |
    // +------------------------------------------+--------------------------------+
    // | 0x00001116 ERRINFO_SECURITYDATATOOSHORT7 | There is not enough data to    |
    // |                                          | read the clientAddressFamily   |
    // |                                          | and cbClientAddress fields in  |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001117 ERRINFO_SECURITYDATATOOSHORT8 | There is not enough data to    |
    // |                                          | read the clientAddress field in|
    // |                                          | the Extended Info Packet       |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001118 ERRINFO_SECURITYDATATOOSHORT9 | There is not enough data to    |
    // |                                          | read the cbClientDir field in  |
    // |                                          | the Extended Info Packet       |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001119 ERRINFO_SECURITYDATATOOSHORT10| There is not enough data to    |
    // |                                          | read the clientDir field in the|
    // |                                          | Extended Info Packet (section  |
    // |                                          | 2.2.1.11.1.1.1).               |
    // +------------------------------------------+--------------------------------+
    // | 0x0000111A ERRINFO_SECURITYDATATOOSHORT11| There is not enough data to    |
    // |                                          | read the clientTimeZone field  |
    // |                                          | in the Extended Info Packet    |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x0000111B ERRINFO_SECURITYDATATOOSHORT12| There is not enough data to    |
    // |                                          | read the clientSessionId field |
    // |                                          | in the Extended Info Packet    |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x0000111C ERRINFO_SECURITYDATATOOSHORT13| There is not enough data to    |
    // |                                          | read the performanceFlags      |
    // |                                          | field in the Extended Info     |
    // |                                          | Packet (section                |
    // |                                          | 2.2.1.11.1.1.1).               |
    // +------------------------------------------+--------------------------------+
    // | 0x0000111D ERRINFO_SECURITYDATATOOSHORT14| There is not enough data to    |
    // |                                          | read the cbAutoReconnectLen    |
    // |                                          | field in the Extended Info     |
    // |                                          | Packet (section                |
    // |                                          | 2.2.1.11.1.1.1).               |
    // +------------------------------------------+--------------------------------+
    // | 0x0000111E ERRINFO_SECURITYDATATOOSHORT15| There is not enough data to    |
    // |                                          | read the autoReconnectCookie   |
    // |                                          | field in the Extended Info     |
    // |                                          | Packet (section                |
    // |                                          | 2.2.1.11.1.1.1).               |
    // +------------------------------------------+--------------------------------+
    // | 0x0000111F ERRINFO_SECURITYDATATOOSHORT16| The cbAutoReconnectLen field   |
    // |                                          | in the Extended Info Packet    |
    // |                                          | (section 2.2.1.11.1.1.1)       |
    // |                                          | contains a value which is      |
    // |                                          | larger than the maximum        |
    // |                                          | allowed length of 128 bytes.   |
    // +------------------------------------------+--------------------------------+
    // | 0x00001120 ERRINFO_SECURITYDATATOOSHORT17| There is not enough data to    |
    // |                                          | read the clientAddressFamily   |
    // |                                          | and cbClientAddress fields in  |
    // |                                          | the Extended Info Packet       |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001121 ERRINFO_SECURITYDATATOOSHORT18| There is not enough data to    |
    // |                                          | read the clientAddress field in|
    // |                                          | the Extended Info Packet       |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001122 ERRINFO_SECURITYDATATOOSHORT19| There is not enough data to    |
    // |                                          | read the cbClientDir field in  |
    // |                                          | the Extended Info Packet       |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001123 ERRINFO_SECURITYDATATOOSHORT20| There is not enough data to    |
    // |                                          | read the clientDir field in    |
    // |                                          | the Extended Info Packet       |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001124 ERRINFO_SECURITYDATATOOSHORT21| There is not enough data to    |
    // |                                          | read the clientTimeZone field  |
    // |                                          | in the Extended Info Packet    |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001125 ERRINFO_SECURITYDATATOOSHORT22| There is not enough data to    |
    // |                                          | read the clientSessionId field |
    // |                                          | in the Extended Info Packet    |
    // |                                          | (section 2.2.1.11.1.1.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001126 ERRINFO_SECURITYDATATOOSHORT23| There is not enough data to    |
    // |                                          | read the Client Info PDU Data  |
    // |                                          | (section 2.2.1.11.1).          |
    // +------------------------------------------+--------------------------------+
    // | 0x00001129 ERRINFO_BADMONITORDATA        | The monitorCount field in the  |
    // |                                          | Client Monitor Data (section   |
    // |                                          | 2.2.1.3.6) is invalid.         |
    // +------------------------------------------+--------------------------------+
    // | 0x0000112A                               | The server-side decompression  |
    // | ERRINFO_VCDECOMPRESSEDREASSEMBLEFAILED   | buffer is invalid, or the size |
    // |                                          | of the decompressed VC data    |
    // |                                          | exceeds the chunking size      |
    // |                                          | specified in the Virtual       |
    // |                                          | Channel Capability Set         |
    // |                                          | (section 2.2.7.1.10).          |
    // +------------------------------------------+--------------------------------+
    // | 0x0000112B ERRINFO_VCDATATOOLONG         | The size of a received Virtual |
    // |                                          | Channel PDU (section 2.2.6.1)  |
    // |                                          | exceeds the chunking size      |
    // |                                          | specified in the Virtual       |
    // |                                          | Channel Capability Set         |
    // |                                          | (section 2.2.7.1.10).          |
    // +------------------------------------------+--------------------------------+
    // | 0x0000112C ERRINFO_BAD_FRAME_ACK_DATA    | There is not enough data to    |
    // |                                          | read a                         |
    // |                                          | TS_FRAME_ACKNOWLEDGE_PDU ([MS- |
    // |                                          | RDPRFX] section 2.2.3.1).      |
    // +------------------------------------------+--------------------------------+
    // | 0x0000112D                               | The graphics mode requested by |
    // | ERRINFO_GRAPHICSMODENOTSUPPORTED         | the client is not supported by |
    // |                                          | the server.                    |
    // +------------------------------------------+--------------------------------+
    // | 0x0000112E                               | The server-side graphics       |
    // | ERRINFO_GRAPHICSSUBSYSTEMRESETFAILED     | subsystem failed to reset.     |
    // +------------------------------------------+--------------------------------+
    // | 0x0000112F                               | The server-side graphics       |
    // | ERRINFO_GRAPHICSSUBSYSTEMFAILED          | subsystem is in an error state |
    // |                                          | and unable to continue         |
    // |                                          | graphics encoding.             |
    // +------------------------------------------+--------------------------------+
    // | 0x00001130                               | There is not enough data to    |
    // | ERRINFO_TIMEZONEKEYNAMELENGTHTOOSHORT    | read the                       |
    // |                                          | cbDynamicDSTTimeZoneKeyName    |
    // |                                          | field in the Extended Info     |
    // |                                          | Packet (section                |
    // |                                          | 2.2.1.11.1.1.1).               |
    // +------------------------------------------+--------------------------------+
    // | 0x00001131                               | The length reported in the     |
    // | ERRINFO_TIMEZONEKEYNAMELENGTHTOOLONG     | cbDynamicDSTTimeZoneKeyName    |
    // |                                          | field of the Extended Info     |
    // |                                          | Packet (section                |
    // |                                          | 2.2.1.11.1.1.1) is too long.   |
    // +------------------------------------------+--------------------------------+
    // | 0x00001132                               | The                            |
    // | ERRINFO_DYNAMICDSTDISABLEDFIELDMISSING   | dynamicDaylightTimeDisabled    |
    // |                                          | field is not present in the    |
    // |                                          | Extended Info Packet (section  |
    // |                                          | 2.2.1.11.1.1.1).               |
    // +------------------------------------------+--------------------------------+
    // | 0x00001191                               | An attempt to update the       |
    // | ERRINFO_UPDATESESSIONKEYFAILED           | session keys while using       |
    // |                                          | Standard RDP Security          |
    // |                                          | mechanisms (section 5.3.7)     |
    // |                                          | failed.                        |
    // +------------------------------------------+--------------------------------+
    // | 0x00001192 ERRINFO_DECRYPTFAILED         | (a) Decryption using Standard  |
    // |                                          | RDP Security mechanisms        |
    // |                                          | (section 5.3.6) failed.        |
    // |                                          | (b) Session key creation using |
    // |                                          | Standard RDP Security          |
    // |                                          | mechanisms (section 5.3.5)     |
    // |                                          | failed.                        |
    // +------------------------------------------+--------------------------------+
    // | 0x00001193 ERRINFO_ENCRYPTFAILED         | Encryption using Standard RDP  |
    // |                                          | Security mechanisms (section   |
    // |                                          | 5.3.6) failed.                 |
    // +------------------------------------------+--------------------------------+
    // | 0x00001194 ERRINFO_ENCPKGMISMATCH        | Failed to find a usable        |
    // |                                          | Encryption Method (section     |
    // |                                          | 5.3.2) in the encryptionMethods|
    // |                                          | field of the Client Security   |
    // |                                          | Data (section 2.2.1.4.3).      |
    // +------------------------------------------+--------------------------------+
    // | 0x00001195 ERRINFO_DECRYPTFAILED2        | Encryption using Standard RDP  |
    // |                                          | Security mechanisms (section   |
    // |                                          | 5.3.6) failed. Unencrypted     |
    // |                                          | data was encountered in a      |
    // |                                          | protocol stream which is meant |
    // |                                          | to be encrypted with Standard  |
    // |                                          | RDP Security mechanisms        |
    // |                                          | (section 5.3.6).               |
    // +------------------------------------------+--------------------------------+

    enum {
        ERRINFO_RPC_INITIATED_DISCONNECT          = 0x00000001,
        ERRINFO_RPC_INITIATED_LOGOFF              = 0x00000002,
        ERRINFO_IDLE_TIMEOUT                      = 0x00000003,
        ERRINFO_LOGON_TIMEOUT                     = 0x00000004,
        ERRINFO_DISCONNECTED_BY_OTHERCONNECTION   = 0x00000005,
        ERRINFO_OUT_OF_MEMORY                     = 0x00000006,
        ERRINFO_SERVER_DENIED_CONNECTION          = 0x00000007,
        ERRINFO_SERVER_INSUFFICIENT_PRIVILEGES    = 0x00000009,
        ERRINFO_SERVER_FRESH_CREDENTIALS_REQUIRED = 0x0000000A,
        ERRINFO_RPC_INITIATED_DISCONNECT_BYUSER   = 0x0000000B,
        ERRINFO_LOGOFF_BY_USER                    = 0x0000000C,
        ERRINFO_LICENSE_INTERNAL                  = 0x00000100,
        ERRINFO_LICENSE_NO_LICENSE_SERVER         = 0x00000101,
        ERRINFO_LICENSE_NO_LICENSE                = 0x00000102,
        ERRINFO_LICENSE_BAD_CLIENT_MSG            = 0x00000103,
        ERRINFO_LICENSE_HWID_DOESNT_MATCH_LICENSE = 0x00000104,
        ERRINFO_LICENSE_BAD_CLIENT_LICENSE        = 0x00000105,
        ERRINFO_LICENSE_CANT_FINISH_PROTOCOL      = 0x00000106,
        ERRINFO_LICENSE_CLIENT_ENDED_PROTOCOL     = 0x00000107,
        ERRINFO_LICENSE_BAD_CLIENT_ENCRYPTION     = 0x00000108,
        ERRINFO_LICENSE_CANT_UPGRADE_LICENSE      = 0x00000109,
        ERRINFO_LICENSE_NO_REMOTE_CONNECTIONS     = 0x0000010A,

        ERRINFO_CB_DESTINATION_NOT_FOUND             = 0x00000400,
        ERRINFO_CB_LOADING_DESTINATION               = 0x00000402,
        ERRINFO_CB_REDIRECTING_TO_DESTINATION        = 0x00000404,
        ERRINFO_CB_SESSION_ONLINE_VM_WAKE            = 0x00000405,
        ERRINFO_CB_SESSION_ONLINE_VM_BOOT            = 0x00000406,
        ERRINFO_CB_SESSION_ONLINE_VM_NO_DNS          = 0x00000407,
        ERRINFO_CB_DESTINATION_POOL_NOT_FREE         = 0x00000408,
        ERRINFO_CB_CONNECTION_CANCELLED              = 0x00000409,
        ERRINFO_CB_CONNECTION_ERROR_INVALID_SETTINGS = 0x00000410,
        ERRINFO_CB_SESSION_ONLINE_VM_BOOT_TIMEOUT    = 0x00000411,
        ERRINFO_CB_SESSION_ONLINE_VM_SESSMON_FAILED  = 0x00000412,

        ERRINFO_UNKNOWNPDUTYPE2                   = 0x000010C9,
        ERRINFO_UNKNOWNPDUTYPE                    = 0x000010CA,
        ERRINFO_DATAPDUSEQUENCE                   = 0x000010CB,
        ERRINFO_CONTROLPDUSEQUENCE                = 0x000010CD,
        ERRINFO_INVALIDCONTROLPDUACTION           = 0x000010CE,
        ERRINFO_INVALIDINPUTPDUTYPE               = 0x000010CF,
        ERRINFO_INVALIDINPUTPDUMOUSE              = 0x000010D0,
        ERRINFO_INVALIDREFRESHRECTPDU             = 0x000010D1,
        ERRINFO_CREATEUSERDATAFAILED              = 0x000010D2,
        ERRINFO_CONNECTFAILED                     = 0x000010D3,
        ERRINFO_CONFIRMACTIVEWRONGSHAREID         = 0x000010D4,
        ERRINFO_CONFIRMACTIVEWRONGORIGINATOR      = 0x000010D5,
        ERRINFO_PERSISTENTKEYPDUBADLENGTH         = 0x000010DA,
        ERRINFO_PERSISTENTKEYPDUILLEGALFIRST      = 0x000010DB,
        ERRINFO_PERSISTENTKEYPDUTOOMANYTOTALKEYS  = 0x000010DC,
        ERRINFO_PERSISTENTKEYPDUTOOMANYCACHEKEYS  = 0x000010DD,
        ERRINFO_INPUTPDUBADLENGTH                 = 0x000010DE,
        ERRINFO_BITMAPCACHEERRORPDUBADLENGTH      = 0x000010DF,
        ERRINFO_SECURITYDATATOOSHORT              = 0x000010E0,
        ERRINFO_VCHANNELDATATOOSHORT              = 0x000010E1,
        ERRINFO_SHAREDATATOOSHORT                 = 0x000010E2,
        ERRINFO_BADSUPRESSOUTPUTPDU               = 0x000010E3,
        ERRINFO_CONFIRMACTIVEPDUTOOSHORT          = 0x000010E5,
        ERRINFO_CAPABILITYSETTOOSMALL             = 0x000010E7,
        ERRINFO_CAPABILITYSETTOOLARGE             = 0x000010E8,
        ERRINFO_NOCURSORCACHE                     = 0x000010E9,
        ERRINFO_BADCAPABILITIES                   = 0x000010EA,
        ERRINFO_VIRTUALCHANNELDECOMPRESSIONERR    = 0x000010EC,
        ERRINFO_INVALIDVCCOMPRESSIONTYPE          = 0x000010ED,
        ERRINFO_INVALIDCHANNELID                  = 0x000010EF,
        ERRINFO_VCHANNELSTOOMANY                  = 0x000010F0,
        ERRINFO_REMOTEAPPSNOTENABLED              = 0x000010F3,
        ERRINFO_CACHECAPNOTSET                    = 0x000010F4,
        ERRINFO_BITMAPCACHEERRORPDUBADLENGTH2     = 0x000010F5,
        ERRINFO_OFFSCRCACHEERRORPDUBADLENGTH      = 0x000010F6,
        ERRINFO_DNGCACHEERRORPDUBADLENGTH         = 0x000010F7,
        ERRINFO_GDIPLUSPDUBADLENGTH               = 0x000010F8,
        ERRINFO_SECURITYDATATOOSHORT2             = 0x00001111,
        ERRINFO_SECURITYDATATOOSHORT3             = 0x00001112,
        ERRINFO_SECURITYDATATOOSHORT4             = 0x00001113,
        ERRINFO_SECURITYDATATOOSHORT5             = 0x00001114,
        ERRINFO_SECURITYDATATOOSHORT6             = 0x00001115,
        ERRINFO_SECURITYDATATOOSHORT7             = 0x00001116,
        ERRINFO_SECURITYDATATOOSHORT8             = 0x00001117,
        ERRINFO_SECURITYDATATOOSHORT9             = 0x00001118,
        ERRINFO_SECURITYDATATOOSHORT10            = 0x00001119,
        ERRINFO_SECURITYDATATOOSHORT11            = 0x0000111A,
        ERRINFO_SECURITYDATATOOSHORT12            = 0x0000111B,
        ERRINFO_SECURITYDATATOOSHORT13            = 0x0000111C,
        ERRINFO_SECURITYDATATOOSHORT14            = 0x0000111D,
        ERRINFO_SECURITYDATATOOSHORT15            = 0x0000111E,
        ERRINFO_SECURITYDATATOOSHORT16            = 0x0000111F,
        ERRINFO_SECURITYDATATOOSHORT17            = 0x00001120,
        ERRINFO_SECURITYDATATOOSHORT18            = 0x00001121,
        ERRINFO_SECURITYDATATOOSHORT19            = 0x00001122,
        ERRINFO_SECURITYDATATOOSHORT20            = 0x00001123,
        ERRINFO_SECURITYDATATOOSHORT21            = 0x00001124,
        ERRINFO_SECURITYDATATOOSHORT22            = 0x00001125,
        ERRINFO_SECURITYDATATOOSHORT23            = 0x00001126,
        ERRINFO_BADMONITORDATA                    = 0x00001129,
        ERRINFO_VCDECOMPRESSEDREASSEMBLEFAILED    = 0x0000112A,
        ERRINFO_VCDATATOOLONG                     = 0x0000112B,
        ERRINFO_BAD_FRAME_ACK_DATA                = 0x0000112C,
        ERRINFO_GRAPHICSMODENOTSUPPORTED          = 0x0000112D,
        ERRINFO_GRAPHICSSUBSYSTEMRESETFAILED      = 0x0000112E,
        ERRINFO_GRAPHICSSUBSYSTEMFAILED           = 0x0000112F,
        ERRINFO_TIMEZONEKEYNAMELENGTHTOOSHORT     = 0x00001130,
        ERRINFO_TIMEZONEKEYNAMELENGTHTOOLONG      = 0x00001131,
        ERRINFO_DYNAMICDSTDISABLEDFIELDMISSING    = 0x00001132,
        ERRINFO_UPDATESESSIONKEYFAILED            = 0x00001191,
        ERRINFO_DECRYPTFAILED                     = 0x00001192,
        ERRINFO_ENCRYPTFAILED                     = 0x00001193,
        ERRINFO_ENCPKGMISMATCH                    = 0x00001194,
        ERRINFO_DECRYPTFAILED2                    = 0x00001195
    };

    void process_disconnect_pdu(InStream & stream) {
        uint32_t errorInfo = stream.in_uint32_le();
        switch (errorInfo){
        case ERRINFO_RPC_INITIATED_DISCONNECT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "RPC_INITIATED_DISCONNECT");
            break;
        case ERRINFO_RPC_INITIATED_LOGOFF:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "RPC_INITIATED_LOGOFF");
            break;
        case ERRINFO_IDLE_TIMEOUT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "IDLE_TIMEOUT");
            break;
        case ERRINFO_LOGON_TIMEOUT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LOGON_TIMEOUT");
            break;
        case ERRINFO_DISCONNECTED_BY_OTHERCONNECTION:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "DISCONNECTED_BY_OTHERCONNECTION");
            if (this->acl) {
                this->acl->set_auth_error_message(TR("disconnected_by_otherconnection", this->lang));
            }
            break;
        case ERRINFO_OUT_OF_MEMORY:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "OUT_OF_MEMORY");
            break;
        case ERRINFO_SERVER_DENIED_CONNECTION:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SERVER_DENIED_CONNECTION");
            break;
        case ERRINFO_SERVER_INSUFFICIENT_PRIVILEGES:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SERVER_INSUFFICIENT_PRIVILEGES");
            break;
        case ERRINFO_SERVER_FRESH_CREDENTIALS_REQUIRED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SERVER_FRESH_CREDENTIALS_REQUIRED");
            break;
        case ERRINFO_RPC_INITIATED_DISCONNECT_BYUSER:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "RPC_INITIATED_DISCONNECT_BYUSER");
            break;
        case ERRINFO_LOGOFF_BY_USER:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LOGOFF_BY_USER");
            break;
        case ERRINFO_LICENSE_INTERNAL:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_INTERNAL");
            break;
        case ERRINFO_LICENSE_NO_LICENSE_SERVER:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_NO_LICENSE_SERVER");
            break;
        case ERRINFO_LICENSE_NO_LICENSE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_NO_LICENSE");
            break;
        case ERRINFO_LICENSE_BAD_CLIENT_MSG:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_BAD_CLIENT_MSG");
            break;
        case ERRINFO_LICENSE_HWID_DOESNT_MATCH_LICENSE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_HWID_DOESNT_MATCH_LICENSE");
            break;
        case ERRINFO_LICENSE_BAD_CLIENT_LICENSE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_BAD_CLIENT_LICENSE");
            break;
        case ERRINFO_LICENSE_CANT_FINISH_PROTOCOL:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_CANT_FINISH_PROTOCOL");
            break;
        case ERRINFO_LICENSE_CLIENT_ENDED_PROTOCOL:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_CLIENT_ENDED_PROTOCOL");
            break;
        case ERRINFO_LICENSE_BAD_CLIENT_ENCRYPTION:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_BAD_CLIENT_ENCRYPTION");
            break;
        case ERRINFO_LICENSE_CANT_UPGRADE_LICENSE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_CANT_UPGRADE_LICENSE");
            break;
        case ERRINFO_LICENSE_NO_REMOTE_CONNECTIONS:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "LICENSE_NO_REMOTE_CONNECTIONS");
            break;
        case ERRINFO_CB_DESTINATION_NOT_FOUND:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_DESTINATION_NOT_FOUND");
            break;
        case ERRINFO_CB_LOADING_DESTINATION:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_LOADING_DESTINATION");
            break;
        case ERRINFO_CB_REDIRECTING_TO_DESTINATION:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_REDIRECTING_TO_DESTINATION");
            break;
        case ERRINFO_CB_SESSION_ONLINE_VM_WAKE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_SESSION_ONLINE_VM_WAKE");
            break;
        case ERRINFO_CB_SESSION_ONLINE_VM_BOOT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_SESSION_ONLINE_VM_BOOT");
            break;
        case ERRINFO_CB_SESSION_ONLINE_VM_NO_DNS:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_SESSION_ONLINE_VM_NO_DNS");
            break;
        case ERRINFO_CB_DESTINATION_POOL_NOT_FREE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_DESTINATION_POOL_NOT_FREE");
            break;
        case ERRINFO_CB_CONNECTION_CANCELLED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_CONNECTION_CANCELLED");
            break;
        case ERRINFO_CB_CONNECTION_ERROR_INVALID_SETTINGS:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_CONNECTION_ERROR_INVALID_SETTINGS");
            break;
        case ERRINFO_CB_SESSION_ONLINE_VM_BOOT_TIMEOUT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_SESSION_ONLINE_VM_BOOT_TIMEOUT");
            break;
        case ERRINFO_CB_SESSION_ONLINE_VM_SESSMON_FAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CB_SESSION_ONLINE_VM_SESSMON_FAILED");
            break;
        case ERRINFO_UNKNOWNPDUTYPE2:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "UNKNOWNPDUTYPE2");
            break;
        case ERRINFO_UNKNOWNPDUTYPE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "UNKNOWNPDUTYPE");
            break;
        case ERRINFO_DATAPDUSEQUENCE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "DATAPDUSEQUENCE");
            break;
        case ERRINFO_CONTROLPDUSEQUENCE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CONTROLPDUSEQUENCE");
            break;
        case ERRINFO_INVALIDCONTROLPDUACTION:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "INVALIDCONTROLPDUACTION");
            break;
        case ERRINFO_INVALIDINPUTPDUTYPE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "INVALIDINPUTPDUTYPE");
            break;
        case ERRINFO_INVALIDINPUTPDUMOUSE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "INVALIDINPUTPDUMOUSE");
            break;
        case ERRINFO_INVALIDREFRESHRECTPDU:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "INVALIDREFRESHRECTPDU");
            break;
        case ERRINFO_CREATEUSERDATAFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CREATEUSERDATAFAILED");
            break;
        case ERRINFO_CONNECTFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CONNECTFAILED");
            break;
        case ERRINFO_CONFIRMACTIVEWRONGSHAREID:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CONFIRMACTIVEWRONGSHAREID");
            break;
        case ERRINFO_CONFIRMACTIVEWRONGORIGINATOR:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CONFIRMACTIVEWRONGORIGINATOR");
            break;
        case ERRINFO_PERSISTENTKEYPDUBADLENGTH:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "PERSISTENTKEYPDUBADLENGTH");
            break;
        case ERRINFO_PERSISTENTKEYPDUILLEGALFIRST:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "PERSISTENTKEYPDUILLEGALFIRST");
            break;
        case ERRINFO_PERSISTENTKEYPDUTOOMANYTOTALKEYS:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "PERSISTENTKEYPDUTOOMANYTOTALKEYS");
            break;
        case ERRINFO_PERSISTENTKEYPDUTOOMANYCACHEKEYS:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "PERSISTENTKEYPDUTOOMANYCACHEKEYS");
            break;
        case ERRINFO_INPUTPDUBADLENGTH:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "INPUTPDUBADLENGTH");
            break;
        case ERRINFO_BITMAPCACHEERRORPDUBADLENGTH:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "BITMAPCACHEERRORPDUBADLENGTH");
            break;
        case ERRINFO_SECURITYDATATOOSHORT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT");
            break;
        case ERRINFO_VCHANNELDATATOOSHORT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "VCHANNELDATATOOSHORT");
            break;
        case ERRINFO_SHAREDATATOOSHORT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SHAREDATATOOSHORT");
            break;
        case ERRINFO_BADSUPRESSOUTPUTPDU:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "BADSUPRESSOUTPUTPDU");
            break;
        case ERRINFO_CONFIRMACTIVEPDUTOOSHORT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CONFIRMACTIVEPDUTOOSHORT");
            break;
        case ERRINFO_CAPABILITYSETTOOSMALL:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CAPABILITYSETTOOSMALL");
            break;
        case ERRINFO_CAPABILITYSETTOOLARGE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CAPABILITYSETTOOLARGE");
            break;
        case ERRINFO_NOCURSORCACHE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "NOCURSORCACHE");
            break;
        case ERRINFO_BADCAPABILITIES:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "BADCAPABILITIES");
            break;
        case ERRINFO_VIRTUALCHANNELDECOMPRESSIONERR:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "VIRTUALCHANNELDECOMPRESSIONERR");
            break;
        case ERRINFO_INVALIDVCCOMPRESSIONTYPE:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "INVALIDVCCOMPRESSIONTYPE");
            break;
        case ERRINFO_INVALIDCHANNELID:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "INVALIDCHANNELID");
            break;
        case ERRINFO_VCHANNELSTOOMANY:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "VCHANNELSTOOMANY");
            break;
        case ERRINFO_REMOTEAPPSNOTENABLED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "REMOTEAPPSNOTENABLED");
            break;
        case ERRINFO_CACHECAPNOTSET:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "CACHECAPNOTSET");
            break;
        case ERRINFO_BITMAPCACHEERRORPDUBADLENGTH2:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "BITMAPCACHEERRORPDUBADLENGTH2");
            break;
        case ERRINFO_OFFSCRCACHEERRORPDUBADLENGTH:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "OFFSCRCACHEERRORPDUBADLENGTH");
            break;
        case ERRINFO_DNGCACHEERRORPDUBADLENGTH:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "DNGCACHEERRORPDUBADLENGTH");
            break;
        case ERRINFO_GDIPLUSPDUBADLENGTH:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "GDIPLUSPDUBADLENGTH");
            break;
        case ERRINFO_SECURITYDATATOOSHORT2:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT2");
            break;
        case ERRINFO_SECURITYDATATOOSHORT3:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT3");
            break;
        case ERRINFO_SECURITYDATATOOSHORT4:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT4");
            break;
        case ERRINFO_SECURITYDATATOOSHORT5:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT5");
            break;
        case ERRINFO_SECURITYDATATOOSHORT6:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT6");
            break;
        case ERRINFO_SECURITYDATATOOSHORT7:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT7");
            break;
        case ERRINFO_SECURITYDATATOOSHORT8:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT8");
            break;
        case ERRINFO_SECURITYDATATOOSHORT9:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT9");
            break;
        case ERRINFO_SECURITYDATATOOSHORT10:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT10");
            break;
        case ERRINFO_SECURITYDATATOOSHORT11:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT11");
            break;
        case ERRINFO_SECURITYDATATOOSHORT12:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT12");
            break;
        case ERRINFO_SECURITYDATATOOSHORT13:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT13");
            break;
        case ERRINFO_SECURITYDATATOOSHORT14:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT14");
            break;
        case ERRINFO_SECURITYDATATOOSHORT15:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT15");
            break;
        case ERRINFO_SECURITYDATATOOSHORT16:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT16");
            break;
        case ERRINFO_SECURITYDATATOOSHORT17:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT17");
            break;
        case ERRINFO_SECURITYDATATOOSHORT18:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT18");
            break;
        case ERRINFO_SECURITYDATATOOSHORT19:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT19");
            break;
        case ERRINFO_SECURITYDATATOOSHORT20:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT20");
            break;
        case ERRINFO_SECURITYDATATOOSHORT21:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT21");
            break;
        case ERRINFO_SECURITYDATATOOSHORT22:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT22");
            break;
        case ERRINFO_SECURITYDATATOOSHORT23:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "SECURITYDATATOOSHORT23");
            break;
        case ERRINFO_BADMONITORDATA:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "BADMONITORDATA");
            break;
        case ERRINFO_VCDECOMPRESSEDREASSEMBLEFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "VCDECOMPRESSEDREASSEMBLEFAILED");
            break;
        case ERRINFO_VCDATATOOLONG:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "VCDATATOOLONG");
            break;
        case ERRINFO_BAD_FRAME_ACK_DATA:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "BAD_FRAME_ACK_DATA");
            break;
        case ERRINFO_GRAPHICSMODENOTSUPPORTED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "GRAPHICSMODENOTSUPPORTED");
            break;
        case ERRINFO_GRAPHICSSUBSYSTEMRESETFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "GRAPHICSSUBSYSTEMRESETFAILED");
            break;
        case ERRINFO_GRAPHICSSUBSYSTEMFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "GRAPHICSSUBSYSTEMFAILED");
            break;
        case ERRINFO_TIMEZONEKEYNAMELENGTHTOOSHORT:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "TIMEZONEKEYNAMELENGTHTOOSHORT");
            break;
        case ERRINFO_TIMEZONEKEYNAMELENGTHTOOLONG:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "TIMEZONEKEYNAMELENGTHTOOLONG");
            break;
        case ERRINFO_DYNAMICDSTDISABLEDFIELDMISSING:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "DYNAMICDSTDISABLEDFIELDMISSING");
            break;
        case ERRINFO_UPDATESESSIONKEYFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "UPDATESESSIONKEYFAILED");
            break;
        case ERRINFO_DECRYPTFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "DECRYPTFAILED");
            break;
        case ERRINFO_ENCRYPTFAILED:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "ENCRYPTFAILED");
            break;
        case ERRINFO_ENCPKGMISMATCH:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "ENCPKGMISMATCH");
            break;
        case ERRINFO_DECRYPTFAILED2:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "DECRYPTFAILED2");
            break;
        default:
            LOG(LOG_INFO, "process disconnect pdu : code = %8x error=%s", errorInfo, "?");
            break;
        }
    }   // process_disconnect_pdu

    void process_logon_info(const char * domain, const char * username) {
        char domain_username_format_0[2048];
        char domain_username_format_1[2048];

        snprintf(domain_username_format_0, sizeof(domain_username_format_0),
            "%s@%s", username, domain);
        snprintf(domain_username_format_1, sizeof(domain_username_format_0),
            "%s\\%s", domain, username);
        //LOG(LOG_INFO,
        //    "Domain username format 0=(%s) Domain username format 1=(%s)",
        //    domain_username_format_0, domain_username_format_0);

        if (this->disconnect_on_logon_user_change &&
            ((strcasecmp(domain, this->domain) || strcasecmp(username, this->username)) &&
             (this->domain[0] ||
              (strcasecmp(domain_username_format_0, this->username) &&
               strcasecmp(domain_username_format_1, this->username) &&
               strcasecmp(username, this->username))))) {
            if (this->error_message) {
                *this->error_message = "Unauthorized logon user change detected!";
            }

            this->end_session_reason  = "OPEN_SESSION_FAILED";
            this->end_session_message = "Unauthorized logon user change detected.";

            LOG(LOG_ERR,
                "Unauthorized logon user change detected on %s (%s%s%s) -> (%s%s%s). "
                    "The session will be disconnected.",
                this->hostname, this->domain,
                (*this->domain ? "\\" : ""),
                this->username, domain,
                ((domain && (*domain)) ? "\\" : ""),
                username);
            throw Error(ERR_RDP_LOGON_USER_CHANGED);
        }

        if (this->acl)
        {
            this->acl->report("OPEN_SESSION_SUCCESSFUL", "OK.");
        }
        this->end_session_reason = "CLOSE_SESSION_SUCCESSFUL";
        this->end_session_message = "OK.";

        if (this->open_session_timeout) {
            this->open_session_timeout_checker.cancel_timeout();

            this->event.reset();
        }

        if (this->enable_session_probe && this->enable_session_probe_loading_mask) {
            this->front.disable_input_event_and_graphics_update(true);
        }
    }

    void process_save_session_info(InStream & stream) {
        RDP::SaveSessionInfoPDUData_Recv ssipdudata(stream);

        switch (ssipdudata.infoType) {
        case RDP::INFOTYPE_LOGON:
        {
            LOG(LOG_INFO, "process save session info : Logon");
            RDP::LogonInfoVersion1_Recv liv1(ssipdudata.payload);

            process_logon_info(reinterpret_cast<char *>(liv1.Domain),
                reinterpret_cast<char *>(liv1.UserName));
        }
        break;
        case RDP::INFOTYPE_LOGON_LONG:
        {
            LOG(LOG_INFO, "process save session info : Logon long");
            RDP::LogonInfoVersion2_Recv liv2(ssipdudata.payload);

            process_logon_info(reinterpret_cast<char *>(liv2.Domain),
                reinterpret_cast<char *>(liv2.UserName));
        }
        break;
        case RDP::INFOTYPE_LOGON_PLAINNOTIFY:
        {
            LOG(LOG_INFO, "process save session info : Logon plainnotify");
            RDP::PlainNotify_Recv pn(ssipdudata.payload);

            if (this->enable_session_probe && this->enable_session_probe_loading_mask) {
                this->front.disable_input_event_and_graphics_update(true);
            }
        }
        break;
        case RDP::INFOTYPE_LOGON_EXTENDED_INFO:
        {
            LOG(LOG_INFO, "process save session info : Logon extended info");
            RDP::LogonInfoExtended_Recv lie(ssipdudata.payload);

            RDP::LogonInfoField_Recv lif(lie.payload);

            if (lie.FieldsPresent & RDP::LOGON_EX_AUTORECONNECTCOOKIE) {
                LOG(LOG_INFO, "process save session info : Auto-reconnect cookie");

                RDP::ServerAutoReconnectPacket_Recv sarp(lif.payload);
            }
            if (lie.FieldsPresent & RDP::LOGON_EX_LOGONERRORS) {
                LOG(LOG_INFO, "process save session info : Logon Errors Info");

                RDP::LogonErrorsInfo_Recv lei(lif.payload);
            }
        }
        break;
        }

        stream.in_skip_bytes(stream.in_remain());
    }

    TODO("CGR: this can probably be unified with process_confirm_active in front")
    void process_server_caps(InStream & stream, uint16_t len) {
        if (this->verbose & 32){
            LOG(LOG_INFO, "mod_rdp::process_server_caps");
        }

        struct autoclose_file {
            FILE * file;

            ~autoclose_file()
            {
                if (this->file) {
                    fclose(this->file);
                }
            }
        };
        FILE * const output_file =
            !this->output_filename.empty()
            ? fopen(this->output_filename.c_str(), "w")
            : nullptr;
        autoclose_file autoclose{output_file};

        unsigned expected = 4; /* numberCapabilities(2) + pad2Octets(2) */
        if (!stream.in_check_rem(expected)){
            LOG(LOG_ERR, "Truncated Demand active PDU data, need=%u remains=%zu",
                expected, stream.in_remain());
            throw Error(ERR_MCS_PDU_TRUNCATED);
        }

        uint16_t ncapsets = stream.in_uint16_le();
        stream.in_skip_bytes(2); /* pad */

        for (uint16_t n = 0; n < ncapsets; n++) {
            expected = 4; /* capabilitySetType(2) + lengthCapability(2) */
            if (!stream.in_check_rem(expected)){
                LOG(LOG_ERR, "Truncated Demand active PDU data, need=%u remains=%zu",
                    expected, stream.in_remain());
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            uint16_t capset_type = stream.in_uint16_le();
            uint16_t capset_length = stream.in_uint16_le();

            expected = capset_length - 4 /* capabilitySetType(2) + lengthCapability(2) */;
            if (!stream.in_check_rem(expected)){
                LOG(LOG_ERR, "Truncated Demand active PDU data, need=%u remains=%zu",
                    expected, stream.in_remain());
                throw Error(ERR_MCS_PDU_TRUNCATED);
            }

            uint8_t const * next = stream.get_current() + expected;
            switch (capset_type) {
            case CAPSTYPE_GENERAL:
                {
                    GeneralCaps general_caps;
                    general_caps.recv(stream, capset_length);
                    if (this->verbose & 1) {
                        general_caps.log("Received from server");
                    }
                    if (output_file) {
                        general_caps.dump(output_file);
                    }
                }
                break;
            case CAPSTYPE_BITMAP:
                {
                    BitmapCaps bitmap_caps;
                    bitmap_caps.recv(stream, capset_length);
                    if (this->verbose & 1) {
                        bitmap_caps.log("Received from server");
                    }
                    if (output_file) {
                        bitmap_caps.dump(output_file);
                    }
                    this->bpp = bitmap_caps.preferredBitsPerPixel;
                    this->front_width = bitmap_caps.desktopWidth;
                    this->front_height = bitmap_caps.desktopHeight;
                }
                break;
            case CAPSTYPE_ORDER:
                {
                    OrderCaps order_caps;
                    order_caps.recv(stream, capset_length);
                    if (this->verbose & 1) {
                        order_caps.log("Received from server");
                    }
                    if (output_file) {
                        order_caps.dump(output_file);
                    }
                }
                break;
            case CAPSTYPE_INPUT:
                {
                    InputCaps input_caps;
                    input_caps.recv(stream, capset_length);
                    if (this->verbose & 1) {
                        input_caps.log("Received from server");
                    }

                    this->enable_fastpath_client_input_event =
                        (this->enable_fastpath && ((input_caps.inputFlags & (INPUT_FLAG_FASTPATH_INPUT | INPUT_FLAG_FASTPATH_INPUT2)) != 0));
                }
                break;
            default:
                if (this->verbose) {
                    LOG(LOG_WARNING,
                        "Unprocessed Capability Set is encountered. capabilitySetType=%s(%u)",
                        ::get_capabilitySetType_name(capset_type),
                        capset_type);
                }
                break;
            }
            stream.in_skip_bytes(next - stream.get_current());
        }

        if (this->verbose & 32){
            LOG(LOG_INFO, "mod_rdp::process_server_caps done");
        }
    }   // process_server_caps

    void send_control(int action) {
        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::send_control");
        }

        this->send_data_request_ex(
            GCC::MCS_GLOBAL_CHANNEL,
            [this, action](StreamSize<256>, OutStream & stream) {
                ShareData sdata(stream);
                sdata.emit_begin(PDUTYPE2_CONTROL, this->share_id, RDP::STREAM_MED);

                // Payload
                stream.out_uint16_le(action);
                stream.out_uint16_le(0); /* userid */
                stream.out_uint32_le(0); /* control id */

                // Packet trailer
                sdata.emit_end();
            },
            [this](StreamSize<256>, OutStream & sctrl_header, std::size_t packet_size) {
                ShareControl_Send(sctrl_header, PDUTYPE_DATAPDU, this->userid + GCC::MCS_USERCHANNEL_BASE, packet_size);

            }
        );

        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::send_control done");
        }
    }

    /* Send persistent bitmap cache enumeration PDU's
       Not implemented yet because it should be implemented
       before in process_data case. The problem is that
       we don't save the bitmap key list attached with rdp_bmpcache2 capability
       message so we can't develop this function yet */
    template<class DataWriter>
    void send_persistent_key_list_pdu(DataWriter data_writer) {
        this->send_pdu_type2(PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST, RDP::STREAM_MED, data_writer);
    }

    template<class DataWriter>
    void send_pdu_type2(uint8_t pdu_type2, uint8_t stream_id, DataWriter data_writer) {
        using packet_size_t = decltype(details_::packet_size(data_writer));
        this->send_data_request_ex(
            GCC::MCS_GLOBAL_CHANNEL,
            [this, &data_writer, pdu_type2, stream_id](
                StreamSize<256 + packet_size_t{}>, OutStream & stream) {
                ShareData sdata(stream);
                sdata.emit_begin(pdu_type2, this->share_id, stream_id);
                {
                    OutStream substream(stream.get_current(), packet_size_t{});
                    data_writer(packet_size_t{}, substream);
                    stream.out_skip_bytes(substream.get_offset());
                }
                sdata.emit_end();
            },
            [this](StreamSize<256>, OutStream & sctrl_header, std::size_t packet_size) {
                ShareControl_Send(
                    sctrl_header, PDUTYPE_DATAPDU,
                    this->userid + GCC::MCS_USERCHANNEL_BASE, packet_size
                );
            }
        );
    }

    void send_persistent_key_list_regular() {
        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::send_persistent_key_list_regular");
        }

        uint16_t totalEntriesCache[BmpCache::MAXIMUM_NUMBER_OF_CACHES] = { 0, 0, 0, 0, 0 };

        for (uint8_t cache_id = 0; cache_id < this->orders.bmp_cache->number_of_cache; cache_id++) {
            const BmpCache::cache_ & cache = this->orders.bmp_cache->get_cache(cache_id);
            if (cache.persistent()) {
                uint16_t idx = 0;
                while (idx < cache.size() && cache[idx]) {
                    ++idx;
                }
                totalEntriesCache[cache_id] = idx;
            }
        }
        //LOG(LOG_INFO, "totalEntriesCache0=%u totalEntriesCache1=%u totalEntriesCache2=%u totalEntriesCache3=%u totalEntriesCache4=%u",
        //    totalEntriesCache[0], totalEntriesCache[1], totalEntriesCache[2], totalEntriesCache[3], totalEntriesCache[4]);

        uint16_t total_number_of_entries = totalEntriesCache[0] + totalEntriesCache[1] + totalEntriesCache[2] +
                                           totalEntriesCache[3] + totalEntriesCache[4];
        if (total_number_of_entries > 0) {
            RDP::PersistentKeyListPDUData pklpdu;
            pklpdu.bBitMask |= RDP::PERSIST_FIRST_PDU;

            uint16_t number_of_entries     = 0;
            uint8_t  pdu_number_of_entries = 0;
            for (uint8_t cache_id = 0; cache_id < this->orders.bmp_cache->number_of_cache; cache_id++) {
                const BmpCache::cache_ & cache = this->orders.bmp_cache->get_cache(cache_id);

                if (!cache.persistent()) {
                    continue;
                }

                const uint16_t entries_max = totalEntriesCache[cache_id];
                for (uint16_t cache_index = 0; cache_index < entries_max; cache_index++) {
                    pklpdu.entries[pdu_number_of_entries].Key1 = cache[cache_index].sig.sig_32[0];
                    pklpdu.entries[pdu_number_of_entries].Key2 = cache[cache_index].sig.sig_32[1];

                    pklpdu.numEntriesCache[cache_id]++;
                    number_of_entries++;
                    pdu_number_of_entries++;

                    if ((pdu_number_of_entries == RDP::PersistentKeyListPDUData::MAXIMUM_ENCAPSULATED_BITMAP_KEYS) ||
                        (number_of_entries == total_number_of_entries))
                    {
                        if (number_of_entries == total_number_of_entries) {
                            pklpdu.bBitMask |= RDP::PERSIST_LAST_PDU;
                        }

                        pklpdu.totalEntriesCache[0] = totalEntriesCache[0];
                        pklpdu.totalEntriesCache[1] = totalEntriesCache[1];
                        pklpdu.totalEntriesCache[2] = totalEntriesCache[2];
                        pklpdu.totalEntriesCache[3] = totalEntriesCache[3];
                        pklpdu.totalEntriesCache[4] = totalEntriesCache[4];

                        //pklpdu.log(LOG_INFO, "Send to server");

                        this->send_persistent_key_list_pdu(
                            [&pklpdu](StreamSize<2048>, OutStream & pdu_data_stream) {
                                pklpdu.emit(pdu_data_stream);
                            }
                        );

                        pklpdu.reset();

                        pdu_number_of_entries = 0;
                    }
                }
            }
        }

        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::send_persistent_key_list_regular done");
        }
    }   // send_persistent_key_list_regular

    void send_persistent_key_list_transparent() {
        if (!this->persistent_key_list_transport) {
            return;
        }

        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::send_persistent_key_list_transparent");
        }

        try
        {
            while (1) {
                this->send_persistent_key_list_pdu(
                    [this](StreamSize<65535>, OutStream & pdu_data_stream) {
                        uint8_t * data = pdu_data_stream.get_data();
                        uint8_t * end = data;
                        this->persistent_key_list_transport->recv(&end, 2/*pdu_size(2)*/);
                        std::size_t pdu_size = Parse(data).in_uint16_le();
                        end = data;
                        this->persistent_key_list_transport->recv(&end, pdu_size);
                        pdu_data_stream.out_skip_bytes(pdu_size);

                        if (this->verbose & 1) {
                            InStream stream(data, pdu_size);
                            RDP::PersistentKeyListPDUData pklpdu;
                            pklpdu.receive(stream);
                            pklpdu.log(LOG_INFO, "Send to server");
                        }
                    }
                );
            }
        }
        catch (Error const & e)
        {
            if (e.id != ERR_TRANSPORT_NO_MORE_DATA) {
                LOG(LOG_ERR, "mod_rdp::send_persistent_key_list_transparent: error=%u", e.id);
                throw;
            }
        }

        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::send_persistent_key_list_transparent done");
        }
    }

    void send_persistent_key_list() {
        if (this->enable_transparent_mode) {
            this->send_persistent_key_list_transparent();
        }
        else {
            this->send_persistent_key_list_regular();
        }
    }

    TODO("CGR: duplicated code in front")
    void send_synchronise() {
        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_synchronise");
        }

        this->send_pdu_type2(
            PDUTYPE2_SYNCHRONIZE, RDP::STREAM_MED,
            [](StreamSize<4>, OutStream & stream) {
                stream.out_uint16_le(1); /* type */
                stream.out_uint16_le(1002);
            }
        );

        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_synchronise done");
        }
    }

    void send_fonts(int seq) {
        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_fonts");
        }

        this->send_pdu_type2(
            PDUTYPE2_FONTLIST, RDP::STREAM_MED,
            [seq](StreamSize<8>, OutStream & stream){
                // Payload
                stream.out_uint16_le(0); /* number of fonts */
                stream.out_uint16_le(0); /* pad? */
                stream.out_uint16_le(seq); /* unknown */
                stream.out_uint16_le(0x32); /* entry size */
            }
        );

        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_fonts done");
        }
    }

public:

    void send_input_slowpath(int time, int message_type, int device_flags, int param1, int param2) {
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::send_input_slowpath");
        }

        this->send_pdu_type2(
            PDUTYPE2_INPUT, RDP::STREAM_HI,
            [&](StreamSize<16>, OutStream & stream){
                // Payload
                stream.out_uint16_le(1); /* number of events */
                stream.out_uint16_le(0);
                stream.out_uint32_le(time);
                stream.out_uint16_le(message_type);
                stream.out_uint16_le(device_flags);
                stream.out_uint16_le(param1);
                stream.out_uint16_le(param2);
            }
        );

        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::send_input_slowpath done");
        }
    }

    void send_input_fastpath(int time, int message_type, uint16_t device_flags, int param1, int param2) {
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::send_input_fastpath");
        }

        write_packets(
            this->nego.trans,
            [&](StreamSize<256>, OutStream & stream) {
                switch (message_type) {
                case RDP_INPUT_SCANCODE:
                    FastPath::KeyboardEvent_Send(stream, device_flags, param1);
                    break;

                case RDP_INPUT_SYNCHRONIZE:
                    FastPath::SynchronizeEvent_Send(stream, param1);
                    break;

                case RDP_INPUT_MOUSE:
                    FastPath::MouseEvent_Send(stream, device_flags, param1, param2);
                    break;

                default:
                    LOG(LOG_ERR, "unsupported fast-path input message type 0x%x", message_type);
                    throw Error(ERR_RDP_FASTPATH);
                }
            },
            [&](StreamSize<256>, OutStream & fastpath_header, uint8_t * packet_data, std::size_t packet_size) {
                FastPath::ClientInputEventPDU_Send out_cie(
                    fastpath_header, packet_data, packet_size, 1,
                    this->encrypt, this->encryptionLevel, this->encryptionMethod
                );
                (void)out_cie;
            }
        );

        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::send_input_fastpath done");
        }
    }

    void send_input(int time, int message_type, int device_flags, int param1, int param2) {
        if (this->enable_fastpath_client_input_event == false) {
            this->send_input_slowpath(time, message_type, device_flags, param1, param2);
        }
        else {
            this->send_input_fastpath(time, message_type, device_flags, param1, param2);
        }
    }

    void rdp_input_invalidate(const Rect & r) override {
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::rdp_input_invalidate");
        }
        if (UP_AND_RUNNING == this->connection_finalization_state) {
            if (!r.isempty()){
                RDP::RefreshRectPDU rrpdu(this->share_id,
                                          this->userid,
                                          this->encryptionLevel,
                                          this->encrypt);

                rrpdu.addInclusiveRect(r.x, r.y, r.x + r.cx - 1, r.y + r.cy - 1);

                rrpdu.emit(this->nego.trans);
            }
        }
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::rdp_input_invalidate done");
        }
    }

    void rdp_input_invalidate2(const DArray<Rect> & vr) override {
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::rdp_input_invalidate 2");
        }
        if ((UP_AND_RUNNING == this->connection_finalization_state)
            && (vr.size() > 0)) {
            RDP::RefreshRectPDU rrpdu(this->share_id,
                                      this->userid,
                                      this->encryptionLevel,
                                      this->encrypt);
            for (size_t i = 0; i < vr.size() ; i++){
                if (!vr[i].isempty()){
                    rrpdu.addInclusiveRect(vr[i].x, vr[i].y, vr[i].x + vr[i].cx - 1, vr[i].y + vr[i].cy - 1);
                }
            }
            rrpdu.emit(this->nego.trans);
        }
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::rdp_input_invalidate done");
        }

    };

    // 2.2.9.1.2.1.7 Fast-Path Color Pointer Update (TS_FP_COLORPOINTERATTRIBUTE)
    // =========================================================================

    // updateHeader (1 byte): An 8-bit, unsigned integer. The format of this field is
    // the same as the updateHeader byte field specified in the Fast-Path Update
    // (section 2.2.9.1.2.1) structure. The updateCode bitfield (4 bits in size) MUST
    // be set to FASTPATH_UPDATETYPE_COLOR (9).

    // compressionFlags (1 byte): An 8-bit, unsigned integer. The format of this optional
    // field (as well as the possible values) is the same as the compressionFlags field
    // specified in the Fast-Path Update structure.

    // size (2 bytes): A 16-bit, unsigned integer. The format of this field (as well as
    // the possible values) is the same as the size field specified in the Fast-Path
    // Update structure.

    // colorPointerUpdateData (variable): Color pointer data. Both slow-path and
    // fast-path utilize the same data format, a Color Pointer Update (section
    // 2.2.9.1.1.4.4) structure, to represent this information.

    // 2.2.9.1.1.4.4 Color Pointer Update (TS_COLORPOINTERATTRIBUTE)
    // =============================================================

    // The TS_COLORPOINTERATTRIBUTE structure represents a regular T.128 24 bpp
    // color pointer, as specified in [T128] section 8.14.3. This pointer update
    // is used for both monochrome and color pointers in RDP.

    //    cacheIndex (2 bytes): A 16-bit, unsigned integer. The zero-based cache
    // entry in the pointer cache in which to store the pointer image. The number
    // of cache entries is specified using the Pointer Capability Set (section 2.2.7.1.5).

    //    hotSpot (4 bytes): Point (section 2.2.9.1.1.4.1 ) structure containing
    // the x-coordinates and y-coordinates of the pointer hotspot.

    //    width (2 bytes): A 16-bit, unsigned integer. The width of the pointer
    // in pixels. The maximum allowed pointer width is 96 pixels if the client
    // indicated support for large pointers by setting the LARGE_POINTER_FLAG (0x00000001)
    // in the Large Pointer Capability Set (section 2.2.7.2.7). If the LARGE_POINTER_FLAG
    // was not set, the maximum allowed pointer width is 32 pixels.

    //    height (2 bytes): A 16-bit, unsigned integer. The height of the pointer
    // in pixels. The maximum allowed pointer height is 96 pixels if the client
    // indicated support for large pointers by setting the LARGE_POINTER_FLAG (0x00000001)
    // in the Large Pointer Capability Set (section 2.2.7.2.7). If the LARGE_POINTER_FLAG
    // was not set, the maximum allowed pointer height is 32 pixels.

    //    lengthAndMask (2 bytes): A 16-bit, unsigned integer. The size in bytes of the
    // andMaskData field.

    //    lengthXorMask (2 bytes): A 16-bit, unsigned integer. The size in bytes of the
    // xorMaskData field.

    //    xorMaskData (variable): A variable-length array of bytes. Contains the 24-bpp,
    // bottom-up XOR mask scan-line data. The XOR mask is padded to a 2-byte boundary for
    // each encoded scan-line. For example, if a 3x3 pixel cursor is being sent, then each
    // scan-line will consume 10 bytes (3 pixels per scan-line multiplied by 3 bytes per pixel,
    // rounded up to the next even number of bytes).

    //    andMaskData (variable): A variable-length array of bytes. Contains the 1-bpp, bottom-up
    // AND mask scan-line data. The AND mask is padded to a 2-byte boundary for each encoded scan-line.
    // For example, if a 7x7 pixel cursor is being sent, then each scan-line will consume 2 bytes
    // (7 pixels per scan-line multiplied by 1 bpp, rounded up to the next even number of bytes).

    //    pad (1 byte): An optional 8-bit, unsigned integer. Padding. Values in this field MUST be ignored.

    void process_color_pointer_pdu(InStream & stream) {
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::process_color_pointer_pdu");
        }
        unsigned pointer_cache_idx = stream.in_uint16_le();
        if (pointer_cache_idx >= (sizeof(this->cursors) / sizeof(this->cursors[0]))) {
            LOG(LOG_ERR, "mod_rdp::process_color_pointer_pdu: index out of bounds");
            throw Error(ERR_RDP_PROCESS_COLOR_POINTER_CACHE_NOT_OK);
        }

        Pointer & cursor = this->cursors[pointer_cache_idx];

        memset(&cursor, 0, sizeof(Pointer));
        cursor.bpp = 24;
        cursor.x      = stream.in_uint16_le();
        cursor.y      = stream.in_uint16_le();
        cursor.width  = stream.in_uint16_le();
        cursor.height = stream.in_uint16_le();
        unsigned mlen  = stream.in_uint16_le(); /* mask length */
        unsigned dlen  = stream.in_uint16_le(); /* data length */

        if ((mlen > sizeof(cursor.mask)) || (dlen > sizeof(cursor.data))) {
            LOG(LOG_ERR,
                "mod_rdp::process_color_pointer_pdu: "
                    "bad length for color pointer mask_len=%u data_len=%u",
                mlen, dlen);
            throw Error(ERR_RDP_PROCESS_COLOR_POINTER_LEN_NOT_OK);
        }
        TODO("this is modifiying cursor in place: we should not do that.");
        memcpy(cursor.data, stream.in_uint8p(dlen), dlen);
        memcpy(cursor.mask, stream.in_uint8p(mlen), mlen);

        this->front.server_set_pointer(cursor);
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::process_color_pointer_pdu done");
        }
    }

    // 2.2.9.1.1.4.6 Cached Pointer Update (TS_CACHEDPOINTERATTRIBUTE)
    // ---------------------------------------------------------------

    // The TS_CACHEDPOINTERATTRIBUTE structure is used to instruct the
    // client to change the current pointer shape to one already present
    // in the pointer cache.

    // cacheIndex (2 bytes): A 16-bit, unsigned integer. A zero-based
    // cache entry containing the cache index of the cached pointer to
    // which the client's pointer MUST be changed. The pointer data MUST
    // have already been cached using either the Color Pointer Update
    // (section 2.2.9.1.1.4.4) or New Pointer Update (section 2.2.9.1.1.4.5).

    void process_cached_pointer_pdu(InStream & stream)
    {
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::process_cached_pointer_pdu");
        }

        TODO("Add check that the idx transmitted is actually an used pointer")
        uint16_t pointer_idx = stream.in_uint16_le();
        if (pointer_idx >= (sizeof(this->cursors) / sizeof(Pointer))) {
            LOG(LOG_ERR,
                "mod_rdp::process_cached_pointer_pdu pointer cache idx overflow (%d)",
                pointer_idx);
            throw Error(ERR_RDP_PROCESS_POINTER_CACHE_NOT_OK);
        }
        struct Pointer & cursor = this->cursors[pointer_idx];
        if (cursor.is_valid()) {
            this->front.server_set_pointer(cursor);
        }
        else {
            LOG(LOG_WARNING,
                "mod_rdp::process_cached_pointer_pdu: incalid cache cell index, use system default. index=%u",
                pointer_idx);
            Pointer cursor(Pointer::POINTER_NORMAL);
            this->front.server_set_pointer(cursor);
        }
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::process_cached_pointer_pdu done");
        }
    }

    // 2.2.9.1.1.4.3 System Pointer Update (TS_SYSTEMPOINTERATTRIBUTE)
    // ---------------------------------------------------------------

    // systemPointerType (4 bytes): A 32-bit, unsigned integer. The type of system pointer.

    // +---------------------------+-----------------------------+
    // |      Value                |      Meaning                |
    // +---------------------------+-----------------------------+
    // | SYSPTR_NULL    0x00000000 | The hidden pointer.         |
    // +---------------------------+-----------------------------+
    // | SYSPTR_DEFAULT 0x00007F00 | The default system pointer. |
    // +---------------------------+-----------------------------+

    void process_system_pointer_pdu(InStream & stream)
    {
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::process_system_pointer_pdu");
        }
        int system_pointer_type = stream.in_uint32_le();
        switch (system_pointer_type) {
        case RDP_NULL_POINTER:
            {
                Pointer cursor;
                memset(cursor.mask, 0xff, sizeof(cursor.mask));
                this->front.server_set_pointer(cursor);
            }
            break;
        default:
            {
                Pointer cursor(Pointer::POINTER_NORMAL);
                this->front.server_set_pointer(cursor);
            }
            break;
        }
        if (this->verbose & 4){
            LOG(LOG_INFO, "mod_rdp::process_system_pointer_pdu done");
        }
    }

    void to_regular_mask(const uint8_t * indata, unsigned mlen, uint8_t bpp, uint8_t * mask, size_t mask_size) {
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::to_regular_mask");
        }

        TODO("check code below: why do we revert mask and pointer when pointer is 1 BPP and not with other color depth ?"
             " Looks fishy, a mask and pointer should always be encoded in the same way, not depending on color depth"
             "difficult to see for symmetrical pointers... check documentation");
        TODO("it may be more efficient to revert cursor after creating it instead of doing it on the fly")
        switch (bpp) {
        case 1 :
        {
            for (unsigned x = 0; x < mlen ; x++) {
                BGRColor px = indata[x];
                // incoming new pointer mask is upside down, revert it
                mask[128 - 4 - (x & 0x7C) + (x & 3)] = px;
            }
        }
        break;
        default:
            memcpy(mask, indata, mlen);
        break;
        }

        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::to_regular_mask");
        }
    }

    void to_regular_pointer(const uint8_t * indata, unsigned dlen, uint8_t bpp, uint8_t * data, size_t target_data_len) {
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::to_regular_pointer");
        }
        switch (bpp) {
        case 1 :
        {
            for (unsigned x = 0; x < dlen ; x ++) {
                BGRColor px = indata[x];
                // target cursor will receive 8 bits input at once
                for (unsigned b = 0 ; b < 8 ; b++) {
                    // incoming new pointer is upside down, revert it
                    uint8_t * bstart = &(data[24 * (128 - 4 - (x & 0xFFFC) + (x & 3))]);
                    // emit all individual bits
                    ::out_bytes_le(bstart,      3, (px & 0x80) ? 0xFFFFFF : 0);
                    ::out_bytes_le(bstart +  3, 3, (px & 0x40) ? 0xFFFFFF : 0);
                    ::out_bytes_le(bstart +  6, 3, (px & 0x20) ? 0xFFFFFF : 0);
                    ::out_bytes_le(bstart +  9, 3, (px & 0x10) ? 0xFFFFFF : 0);
                    ::out_bytes_le(bstart + 12, 3, (px &    8) ? 0xFFFFFF : 0);
                    ::out_bytes_le(bstart + 15, 3, (px &    4) ? 0xFFFFFF : 0);
                    ::out_bytes_le(bstart + 18, 3, (px &    2) ? 0xFFFFFF : 0);
                    ::out_bytes_le(bstart + 21, 3, (px &    1) ? 0xFFFFFF : 0);
                }
            }
        }
        break;
        case 4 :
        {
            for (unsigned i = 0; i < dlen ; i++) {
                BGRColor px = indata[i];
                // target cursor will receive 8 bits input at once
                ::out_bytes_le(&(data[6 * i]),     3, color_decode((px >> 4) & 0xF, bpp, this->orders.global_palette));
                ::out_bytes_le(&(data[6 * i + 3]), 3, color_decode(px        & 0xF, bpp, this->orders.global_palette));
            }
        }
        break;
        case 32: case 24: case 16: case 15: case 8:
        {
            uint8_t BPP = nbbytes(bpp);
            for (unsigned i = 0; i + BPP <= dlen; i += BPP) {
                BGRColor px = in_uint32_from_nb_bytes_le(BPP, indata + i);
                ::out_bytes_le(&(data[(i/BPP)*3]), 3, color_decode(px, bpp, this->orders.global_palette));
            }
        }
        break;
        default:
            LOG(LOG_ERR, "Mouse pointer : color depth not supported %d, forcing green mouse (running in the grass ?)", bpp);
            for (size_t x = 0 ; x < 1024 ; x++) {
                ::out_bytes_le(data + x *3, 3, GREEN);
            }
            break;
        }

        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::to_regular_pointer");
        }
    }

    // 2.2.9.1.1.4.5 New Pointer Update (TS_POINTERATTRIBUTE)
    // ------------------------------------------------------

    // The TS_POINTERATTRIBUTE structure is used to send pointer data at an arbitrary
    // color depth. Support for the New Pointer Update is advertised in the Pointer
    // Capability Set (section 2.2.7.1.5).


    // xorBpp (2 bytes): A 16-bit, unsigned integer. The color depth in bits-per-pixel
    // of the XOR mask contained in the colorPtrAttr field.

    // colorPtrAttr (variable): Encapsulated Color Pointer Update (section 2.2.9.1.1.4.4)
    //  structure which contains information about the pointer. The Color Pointer Update
    //  fields are all used, as specified in section 2.2.9.1.1.4.4; however color XOR data
    //  is presented in the color depth described in the xorBpp field (for 8 bpp, each byte
    //  contains one palette index; for 4 bpp, there are two palette indices per byte).

    void process_new_pointer_pdu(InStream & stream) {
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::process_new_pointer_pdu");
        }

        unsigned data_bpp  = stream.in_uint16_le(); /* data bpp */
        unsigned pointer_idx = stream.in_uint16_le();

        if (pointer_idx >= (sizeof(this->cursors) / sizeof(Pointer))) {
            LOG(LOG_ERR,
                "mod_rdp::process_new_pointer_pdu pointer cache idx overflow (%d)",
                pointer_idx);
            throw Error(ERR_RDP_PROCESS_POINTER_CACHE_NOT_OK);
        }

        Pointer & cursor = this->cursors[pointer_idx];
        memset(&cursor, 0, sizeof(struct Pointer));
        cursor.bpp    = 24;
        cursor.x      = stream.in_uint16_le();
        cursor.y      = stream.in_uint16_le();
        cursor.width  = stream.in_uint16_le();
        cursor.height = stream.in_uint16_le();
        uint16_t mlen  = stream.in_uint16_le(); /* mask length */
        uint16_t dlen  = stream.in_uint16_le(); /* data length */

        if (cursor.width > Pointer::MAX_WIDTH){
            LOG(LOG_ERR, "mod_rdp::process_new_pointer_pdu pointer width overflow (%d)", cursor.width);
            throw Error(ERR_RDP_PROCESS_POINTER_CACHE_NOT_OK);
        }
        if (cursor.height > Pointer::MAX_HEIGHT){
            LOG(LOG_ERR, "mod_rdp::process_new_pointer_pdu pointer height overflow (%d)", cursor.height);
            throw Error(ERR_RDP_PROCESS_POINTER_CACHE_NOT_OK);
        }

        if (static_cast<unsigned>(cursor.x) >= cursor.width){
            LOG(LOG_INFO, "mod_rdp::process_new_pointer_pdu hotspot x out of pointer (%d >= %d)", cursor.x, cursor.width);
            cursor.x = 0;
        }

        if (static_cast<unsigned>(cursor.y) >= cursor.height){
            LOG(LOG_INFO, "mod_rdp::process_new_pointer_pdu hotspot y out of pointer (%d >= %d)", cursor.y, cursor.height);
            cursor.y = 0;
        }

        if (!stream.in_check_rem(dlen)){
            LOG(LOG_ERR, "Not enough data for cursor pixels (need=%" PRIu16 " remain=%zu)",
                dlen, stream.in_remain());
            throw Error(ERR_RDP_PROCESS_NEW_POINTER_LEN_NOT_OK);
        }
        if (!stream.in_check_rem(mlen + dlen)){
            LOG(LOG_ERR, "Not enough data for cursor mask (need=%" PRIu16 " remain=%zu)",
                mlen, stream.in_remain() - dlen);
            throw Error(ERR_RDP_PROCESS_NEW_POINTER_LEN_NOT_OK);
        }

        size_t out_data_len = 3 * (
            (bpp == 1) ? (cursor.width * cursor.height) / 8 :
            (bpp == 4) ? (cursor.width * cursor.height) / 2 :
            (dlen / nbbytes(data_bpp)));

        if ((mlen > sizeof(cursor.mask)) ||
            (out_data_len > sizeof(cursor.data))) {
            LOG(LOG_ERR,
                "mod_rdp::Bad length for color pointer mask_len=%" PRIu16 " "
                    "data_len=%" PRIu16 " Width = %u Height = %u bpp = %u out_data_len = %zu nbbytes=%" PRIu8,
                mlen, dlen, cursor.width, cursor.height,
                data_bpp, out_data_len, nbbytes(data_bpp));
            throw Error(ERR_RDP_PROCESS_NEW_POINTER_LEN_NOT_OK);
        }

        if (data_bpp == 1) {
            uint8_t data_data[32*32/8];
            uint8_t mask_data[32*32/8];
            stream.in_copy_bytes(data_data, dlen);
            stream.in_copy_bytes(mask_data, mlen);

            for (unsigned i = 0 ; i < mlen; i++) {
                uint8_t new_mask_data = (mask_data[i] & (data_data[i] ^ 0xFF));
                uint8_t new_data_data = (data_data[i] ^ mask_data[i] ^ new_mask_data);
                data_data[i]    = new_data_data;
                mask_data[i]    = new_mask_data;
            }

            TODO("move that into cursor")
            this->to_regular_pointer(data_data, dlen, 1, cursor.data, sizeof(cursor.data));
            this->to_regular_mask(mask_data, mlen, 1, cursor.mask, sizeof(cursor.mask));
        }
        else {
            TODO("move that into cursor")
            this->to_regular_pointer(stream.get_current(), dlen, data_bpp, cursor.data, sizeof(cursor.data));
            stream.in_skip_bytes(dlen);
            this->to_regular_mask(stream.get_current(), mlen, data_bpp, cursor.mask, sizeof(cursor.mask));
            stream.in_skip_bytes(mlen);
        }

        this->front.server_set_pointer(cursor);
        if (this->verbose & 4) {
            LOG(LOG_INFO, "mod_rdp::process_new_pointer_pdu done");
        }
    }   // process_new_pointer_pdu

    void process_bitmap_updates(InStream & stream, bool fast_path) {
        if (this->verbose & 64){
            LOG(LOG_INFO, "mod_rdp::process_bitmap_updates");
        }

        this->recv_bmp_update++;

        if (fast_path) {
            stream.in_skip_bytes(2); // updateType(2)
        }

        // RDP-BCGR: 2.2.9.1.1.3.1.2 Bitmap Update (TS_UPDATE_BITMAP)
        // ----------------------------------------------------------
        // The TS_UPDATE_BITMAP structure contains one or more rectangular
        // clippings taken from the server-side screen frame buffer (see [T128]
        // section 8.17).

        // shareDataHeader (18 bytes): Share Data Header (section 2.2.8.1.1.1.2)
        // containing information about the packet. The type subfield of the
        // pduType field of the Share Control Header (section 2.2.8.1.1.1.1)
        // MUST be set to PDUTYPE_DATAPDU (7). The pduType2 field of the Share
        // Data Header MUST be set to PDUTYPE2_UPDATE (2).

        // bitmapData (variable): The actual bitmap update data, as specified in
        // section 2.2.9.1.1.3.1.2.1.

        // 2.2.9.1.1.3.1.2.1 Bitmap Update Data (TS_UPDATE_BITMAP_DATA)
        // ------------------------------------------------------------
        // The TS_UPDATE_BITMAP_DATA structure encapsulates the bitmap data that
        // defines a Bitmap Update (section 2.2.9.1.1.3.1.2).

        // updateType (2 bytes): A 16-bit, unsigned integer. The graphics update
        // type. This field MUST be set to UPDATETYPE_BITMAP (0x0001).

        // numberRectangles (2 bytes): A 16-bit, unsigned integer.
        // The number of screen rectangles present in the rectangles field.
        size_t numberRectangles = stream.in_uint16_le();
        if (this->verbose & 64){
            LOG(LOG_INFO, "/* ---------------- Sending %zu rectangles ----------------- */", numberRectangles);
        }

        for (size_t i = 0; i < numberRectangles; i++) {

            // rectangles (variable): Variable-length array of TS_BITMAP_DATA
            // (section 2.2.9.1.1.3.1.2.2) structures, each of which contains a
            // rectangular clipping taken from the server-side screen frame buffer.
            // The number of screen clippings in the array is specified by the
            // numberRectangles field.

            // 2.2.9.1.1.3.1.2.2 Bitmap Data (TS_BITMAP_DATA)
            // ----------------------------------------------

            // The TS_BITMAP_DATA structure wraps the bitmap data bytestream
            // for a screen area rectangle containing a clipping taken from
            // the server-side screen frame buffer.

            // A 16-bit, unsigned integer. Left bound of the rectangle.

            // A 16-bit, unsigned integer. Top bound of the rectangle.

            // A 16-bit, unsigned integer. Right bound of the rectangle.

            // A 16-bit, unsigned integer. Bottom bound of the rectangle.

            // A 16-bit, unsigned integer. The width of the rectangle.

            // A 16-bit, unsigned integer. The height of the rectangle.

            // A 16-bit, unsigned integer. The color depth of the rectangle
            // data in bits-per-pixel.

            // CGR: As far as I understand we should have
            // align4(right-left) == width and bottom-top == height
            // maybe put some assertion to check it's true
            // LOG(LOG_ERR, "left=%u top=%u right=%u bottom=%u width=%u height=%u bpp=%u", left, top, right, bottom, width, height, bpp);

            // A 16-bit, unsigned integer. The flags describing the format
            // of the bitmap data in the bitmapDataStream field.

            // +-----------------------------------+---------------------------+
            // | 0x0001 BITMAP_COMPRESSION         | Indicates that the bitmap |
            // |                                   | data is compressed. This  |
            // |                                   | implies that the          |
            // |                                   | bitmapComprHdr field is   |
            // |                                   | present if the NO_BITMAP_C|
            // |                                   |OMPRESSION_HDR (0x0400)    |
            // |                                   | flag is not set.          |
            // +-----------------------------------+---------------------------+
            // | 0x0400 NO_BITMAP_COMPRESSION_HDR  | Indicates that the        |
            // |                                   | bitmapComprHdr field is   |
            // |                                   | not present(removed for   |
            // |                                   | bandwidth efficiency to   |
            // |                                   | save 8 bytes).            |
            // +-----------------------------------+---------------------------+

            RDPBitmapData bmpdata;

            bmpdata.receive(stream);

            Rect boundary( bmpdata.dest_left
                           , bmpdata.dest_top
                           , bmpdata.dest_right - bmpdata.dest_left + 1
                           , bmpdata.dest_bottom - bmpdata.dest_top + 1
                           );

            // BITMAP_COMPRESSION 0x0001
            // Indicates that the bitmap data is compressed. This implies
            // that the bitmapComprHdr field is present if the
            // NO_BITMAP_COMPRESSION_HDR (0x0400) flag is not set.

            if (this->verbose & 64) {
                LOG( LOG_INFO
                     , "/* Rect [%zu] bpp=%" PRIu16
                       " width=%" PRIu16 " height=%" PRIu16
                       " b(%" PRId16 ", %" PRId16 ", %" PRIu16 ", %" PRIu16 ") */"
                     , i
                     , bmpdata.bits_per_pixel
                     , bmpdata.width
                     , bmpdata.height
                     , boundary.x
                     , boundary.y
                     , boundary.cx
                     , boundary.cy
                     );
            }

            // bitmapComprHdr (8 bytes): Optional Compressed Data Header
            // structure (see Compressed Data Header (TS_CD_HEADER)
            // (section 2.2.9.1.1.3.1.2.3)) specifying the bitmap data
            // in the bitmapDataStream. This field MUST be present if
            // the BITMAP_COMPRESSION (0x0001) flag is present in the
            // Flags field, but the NO_BITMAP_COMPRESSION_HDR (0x0400)
            // flag is not.

            if (bmpdata.flags & BITMAP_COMPRESSION) {
                if ((bmpdata.width <= 0) || (bmpdata.height <= 0)) {
                    LOG( LOG_WARNING
                         , "Unexpected bitmap size: width=%" PRIu16 " height=%" PRIu16 " size=%" PRIu16
                           " left=%" PRIu16 ", top=%" PRIu16 ", right=%" PRIu16 ", bottom=%" PRIu16
                         , bmpdata.width
                         , bmpdata.height
                         , bmpdata.cb_comp_main_body_size
                         , bmpdata.dest_left
                         , bmpdata.dest_top
                         , bmpdata.dest_right
                         , bmpdata.dest_bottom
                         );
                }
            }

            TODO("CGR: check which sanity checks should be done");
                //            if (bufsize != bitmap.bmp_size){
                //                LOG(LOG_WARNING, "Unexpected bufsize in bitmap received [%u != %u] width=%u height=%u bpp=%u",
                //                    bufsize, bitmap.bmp_size, width, height, bpp);
                //            }
                const uint8_t * data = stream.in_uint8p(bmpdata.bitmap_size());
            Bitmap bitmap( this->bpp
                           , bmpdata.bits_per_pixel
                           , &this->orders.global_palette
                           , bmpdata.width
                           , bmpdata.height
                           , data
                           , bmpdata.bitmap_size()
                           , (bmpdata.flags & BITMAP_COMPRESSION)
                           );

            if (   bmpdata.cb_scan_width
                   && ((bmpdata.cb_scan_width - bitmap.line_size()) >= nbbytes(bitmap.bpp()))) {
                LOG( LOG_WARNING
                     , "Bad line size: line_size=%" PRIu16 " width=%" PRIu16 " height=%" PRIu16 " bpp=%" PRIu16
                     , bmpdata.cb_scan_width
                     , bmpdata.width
                     , bmpdata.height
                     , bmpdata.bits_per_pixel
                     );
            }

            if (   bmpdata.cb_uncompressed_size
                   && (bmpdata.cb_uncompressed_size != bitmap.bmp_size())) {
                LOG( LOG_WARNING
                     , "final_size should be size of decompressed bitmap [%" PRIu16 " != %zu]"
                       " width=%" PRIu16 " height=%" PRIu16 " bpp=%" PRIu16
                     , bmpdata.cb_uncompressed_size
                     , bitmap.bmp_size()
                     , bmpdata.width
                     , bmpdata.height
                     , bmpdata.bits_per_pixel
                     );
            }

            this->gd->draw(bmpdata, data, bmpdata.bitmap_size(), bitmap);
        }
        if (this->verbose & 64){
            LOG(LOG_INFO, "mod_rdp::process_bitmap_updates done");
        }
    }   // process_bitmap_updates

    void send_client_info_pdu(int userid, const char * password) {
        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_client_info_pdu");
        }

        InfoPacket infoPacket( this->use_rdp5
                             , this->domain
                             , this->username
                             , password
                             , this->program
                             , this->directory
                             , this->performanceFlags
                             , this->clientAddr
                             );

        this->send_data_request(
            GCC::MCS_GLOBAL_CHANNEL,
            [this, password, &infoPacket](StreamSize<1024>, OutStream & stream) {
                if (this->rdp_compression) {
                    infoPacket.flags |= INFO_COMPRESSION;
                    infoPacket.flags &= ~CompressionTypeMask;
                    infoPacket.flags |= ((this->rdp_compression - 1) << 9);
                }

                if (this->enable_session_probe) {
                    infoPacket.flags &= ~INFO_MAXIMIZESHELL;
                }

                if (this->remote_program) {
                    infoPacket.flags |= INFO_RAIL;
                }

                infoPacket.emit(stream);
            },
            write_sec_send_fn{SEC::SEC_INFO_PKT, this->encrypt, this->encryptionLevel}
        );

        if (this->verbose & 1) {
            infoPacket.log("Send data request", this->password_printing_mode, !this->enable_session_probe);
        }

        if (this->open_session_timeout) {
            this->open_session_timeout_checker.restart_timeout(
                time(nullptr), this->open_session_timeout);
            this->event.set(1000000);
        }

        if (this->verbose & 1){
            LOG(LOG_INFO, "mod_rdp::send_client_info_pdu done");
        }
    }

    void begin_update() override {
        this->front.begin_update();
    }

    void end_update() override {
        this->front.end_update();
    }

    void draw(const RDPOpaqueRect & cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPScrBlt & cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPDestBlt & cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPMultiDstBlt & cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPMultiOpaqueRect & cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDP::RDPMultiPatBlt & cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDP::RDPMultiScrBlt & cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPPatBlt & cmd, const Rect &clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPMemBlt & cmd, const Rect & clip,
                      const Bitmap & bmp) override {
        this->front.draw(cmd, clip, bmp);
    }

    void draw(const RDPMem3Blt & cmd, const Rect & clip,
                      const Bitmap & bmp) override {
        this->front.draw(cmd, clip, bmp);
    }

    void draw(const RDPLineTo& cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPGlyphIndex & cmd, const Rect & clip,
                      const GlyphCache * gly_cache) override {
        this->front.draw(cmd, clip, gly_cache);
    }

    void draw(const RDPPolygonSC& cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPPolygonCB& cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }


    void draw(const RDPPolyline& cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPEllipseSC& cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void draw(const RDPEllipseCB& cmd, const Rect & clip) override {
        this->front.draw(cmd, clip);
    }

    void server_set_pointer(const Pointer & cursor) override {
        this->front.server_set_pointer(cursor);
    }

    void draw(const RDPColCache & cmd) override {
        this->front.draw(cmd);
    }

    void draw(const RDP::FrameMarker & order) override {
        this->front.draw(order);
    }

    void draw(const RDPBitmapData & bitmap_data, const uint8_t * data,
                      size_t size, const Bitmap & bmp) override {
        this->front.draw(bitmap_data, data, size, bmp);
    }

    void draw(const RDPBrushCache & cmd) override {
        this->front.draw(cmd);
    }

    void draw(const RDP::RAIL::NewOrExistingWindow & order) override {
        this->front.draw(order);
    }

    void draw(const RDP::RAIL::WindowIcon & order) override {
        this->front.draw(order);
    }

    void draw(const RDP::RAIL::CachedIcon & order) override {
        this->front.draw(order);
    }

    void draw(const RDP::RAIL::DeletedWindow & order) override {
        this->front.draw(order);
    }

    bool is_up_and_running() override {
        return (UP_AND_RUNNING == this->connection_finalization_state);
    }

    void disconnect() override {
        if (this->is_up_and_running()) {
            if (this->verbose & 1){
                LOG(LOG_INFO, "mod_rdp::disconnect()");
            }
            // this->send_shutdown_request();
            // this->draw_event(time(nullptr));
            this->send_disconnect_ultimatum();
        }
        if (this->acl) {
            this->acl->log4(false, "SESSION_ENDED_BY_PROXY");
        }
    }

    //void send_shutdown_request() {
    //    LOG(LOG_INFO, "SEND SHUTDOWN REQUEST PDU");
    //
    //    BStream stream(65536);
    //    ShareData sdata(stream);
    //    sdata.emit_begin(PDUTYPE2_SHUTDOWN_REQUEST, this->share_id,
    //                     RDP::STREAM_MED);
    //    sdata.emit_end();
    //    BStream sctrl_header(256);
    //    ShareControl_Send(sctrl_header, PDUTYPE_DATAPDU,
    //                      this->userid + GCC::MCS_USERCHANNEL_BASE,
    //                      stream.size());
    //    HStream target_stream(1024, 65536);
    //    target_stream.out_copy_bytes(sctrl_header);
    //    target_stream.out_copy_bytes(stream);
    //    target_stream.mark_end();
    //
    //    this->send_data_request_ex(GCC::MCS_GLOBAL_CHANNEL, target_stream);
    //}

    void send_disconnect_ultimatum() {
        if (this->verbose & 1){
            LOG(LOG_INFO, "SEND MCS DISCONNECT PROVIDER ULTIMATUM PDU");
        }
        write_packets(
            this->nego.trans,
            [](StreamSize<256>, OutStream & mcs_data) {
                MCS::DisconnectProviderUltimatum_Send(mcs_data, 3, MCS::PER_ENCODING);
            },
            write_x224_dt_tpdu_fn{}
        );
    }

    //void send_flow_response_pdu(uint8_t flow_id, uint8_t flow_number) {
    //    LOG(LOG_INFO, "SEND FLOW RESPONSE PDU n° %u", flow_number);
    //    BStream flowpdu(256);
    //    FlowPDU_Send(flowpdu, FLOW_RESPONSE_PDU, flow_id, flow_number,
    //                 this->userid + GCC::MCS_USERCHANNEL_BASE);
    //    HStream target_stream(1024, 65536);
    //    target_stream.out_copy_bytes(flowpdu);
    //    target_stream.mark_end();
    //    this->send_data_request_ex(GCC::MCS_GLOBAL_CHANNEL, target_stream);
    //}

    void process_auth_event(const CHANNELS::ChannelDef & auth_channel,
            InStream & stream, uint32_t length, uint32_t flags, size_t chunk_size) {
        REDASSERT(stream.in_remain() == chunk_size);

        std::string auth_channel_message(char_ptr_cast(stream.get_current()), stream.in_remain());

        LOG(LOG_INFO, "Auth channel data=\"%s\"", auth_channel_message.c_str());

        this->auth_channel_flags  = flags;
        this->auth_channel_chanid = auth_channel.chanid;

        if (this->acl) {
            this->acl->set_auth_channel_target(auth_channel_message.c_str());
        }
    }

    void process_session_probe_event(const CHANNELS::ChannelDef & session_probe_channel,
            InStream & stream, uint32_t length, uint32_t flags, size_t chunk_size) {
        REDASSERT(stream.in_remain() == chunk_size);

        uint16_t message_length = stream.in_uint16_le();
        REDASSERT(message_length == stream.in_remain());
        (void)message_length; // disable -Wunused-variable if REDASSERT is disable
        std::string session_probe_channel_message(char_ptr_cast(stream.get_current()), stream.in_remain());

        while (session_probe_channel_message.back() == '\0') session_probe_channel_message.pop_back();

        const char request_outbound_connection_monitoring_rule[] = "Request=Get outbound connection monitoring rule\x01";

        if (!session_probe_channel_message.compare("Request=Get startup application")) {
            if (this->verbose & 1) {
                LOG(LOG_INFO, "Session Probe channel data=\"%s\"", session_probe_channel_message.c_str());
            }

            if (this->verbose & 1) {
                LOG(LOG_INFO, "Session Probe is ready.");
            }

            this->front.session_probe_started();

            if (this->enable_session_probe_loading_mask && this->front.disable_input_event_and_graphics_update(false)) {
                if (this->verbose & 1) {
                    LOG(LOG_INFO, "Force full screen update. Rect=(0, 0, %u, %u)",
                        this->front_width, this->front_height);
                }
                this->rdp_input_invalidate(Rect(0, 0, this->front_width, this->front_height));
            }

            this->session_probe_is_ready = true;

            FileSystemVirtualChannel& file_system_virtual_channel =
                this->get_file_system_virtual_channel();

            this->file_system_drive_manager.DisableSessionProbeDrive(
                file_system_virtual_channel.to_server_sender,
                this->verbose);

            {
                StaticOutStream<32768> out_s;

                const size_t message_length_offset = out_s.get_offset();
                out_s.out_clear_bytes(sizeof(uint16_t));

                {
                    char cstr[] = "StartupApplication=";
                    out_s.out_copy_bytes(cstr, sizeof(cstr)/sizeof(cstr[0]) - 1);
                }

                if (this->real_alternate_shell.empty()) {
                    char cstr[] = "[Windows Explorer]";
                    out_s.out_copy_bytes(cstr, sizeof(cstr)/sizeof(cstr[0]) - 1);
                }
                else {
                    if (!this->real_working_dir.empty()) {
                        out_s.out_copy_bytes(this->real_working_dir.data(), this->real_working_dir.size());
                    }
                    out_s.out_uint8('\x01');
                    out_s.out_copy_bytes(this->real_alternate_shell.data(), this->real_alternate_shell.size());
                }
                out_s.out_clear_bytes(1);   // Null character

                out_s.set_out_uint16_le(
                    out_s.get_offset() - message_length_offset - sizeof(uint16_t),
                    message_length_offset);

                this->send_to_channel(
                    session_probe_channel, out_s.get_data(), out_s.get_offset(), out_s.get_offset(),
                    CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST
                );
            }


            this->session_probe_event.reset();

            if (this->session_probe_keepalive_timeout > 0) {
                {
                    StaticOutStream<1024> out_s;

                    const size_t message_length_offset = out_s.get_offset();
                    out_s.out_clear_bytes(sizeof(uint16_t));

                    {
                        char cstr[] = "Request=Keep-Alive";
                        out_s.out_copy_bytes(cstr, sizeof(cstr)/sizeof(cstr[0]) - 1);
                    }
                    out_s.out_clear_bytes(1);   // Null character

                    out_s.set_out_uint16_le(
                        out_s.get_offset() - message_length_offset - sizeof(uint16_t),
                        message_length_offset);

                    this->send_to_channel(
                        session_probe_channel, out_s.get_data(), out_s.get_offset(), out_s.get_offset(),
                        CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST
                    );
                }

                this->session_probe_event.set(this->session_probe_keepalive_timeout * 1000);
            }
        }
        else if (!session_probe_channel_message.compare(
                     0,
                     sizeof(request_outbound_connection_monitoring_rule) - 1,
                     request_outbound_connection_monitoring_rule)) {
            const char * remaining_data =
                (session_probe_channel_message.c_str() +
                 sizeof(request_outbound_connection_monitoring_rule) - 1);

            const unsigned int rule_index = ::strtoul(remaining_data, nullptr, 10);

            // OutboundConnectionMonitoringRule=RuleIndex\x01ErrorCode[\x01RuleType\x01HostAddrOrSubnet\x01Port]
            // RuleType  : 0 - notify, 1 - kill.
            // ErrorCode : 0 on success. -1 if an error occurred.

            StaticOutStream<32768> out_s;

            const size_t message_length_offset = out_s.get_offset();
            out_s.out_clear_bytes(sizeof(uint16_t));

            {
                const char cstr[] = "OutboundConnectionMonitoringRule=";
                out_s.out_copy_bytes(cstr, sizeof(cstr) / sizeof(cstr[0]) - 1);
            }

            unsigned int type = 0;
            std::string  host_address_or_subnet;
            unsigned int port = 0;

            bool result = this->outbound_connection_monitor_rules.get(
                rule_index, type, host_address_or_subnet, port);

            {
                const int error_code = (result ? 0 : -1);
                char cstr[128];
                snprintf(cstr, sizeof(cstr), "%u" "\x01" "%d" "\x01", rule_index, error_code);
                out_s.out_copy_bytes(cstr, strlen(cstr));
            }

            if (result)
            {
                char cstr[1024];
                snprintf(cstr, sizeof(cstr), "%u\x01%s\x01%u", type, host_address_or_subnet.c_str(), port);
                out_s.out_copy_bytes(cstr, strlen(cstr));
            }

            out_s.out_clear_bytes(1);   // Null character

            out_s.set_out_uint16_le(
                out_s.get_offset() - message_length_offset - sizeof(uint16_t),
                message_length_offset);

            this->send_to_channel(
                session_probe_channel, out_s.get_data(), out_s.get_offset(), out_s.get_offset(),
                CHANNELS::CHANNEL_FLAG_FIRST | CHANNELS::CHANNEL_FLAG_LAST
            );
        }
        else if (!session_probe_channel_message.compare("KeepAlive=OK")) {
            if (this->verbose & 0x10000) {
                LOG(LOG_INFO, "Recevied Keep-Alive from Session Probe.");
            }
            this->session_probe_keep_alive_received = true;
        }
        else {
            const char * message   = session_probe_channel_message.c_str();
            const char * separator = ::strchr(message, '=');

            bool message_format_invalid = false;

            if (separator) {
                std::string order(message, separator - message);
                std::string parameters(separator + 1);

                if (!order.compare("PasswordTextBox.SetFocus")) {
                    std::string info("status=\"" + parameters + "\"");
                    this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());

                    this->front.set_focus_on_password_textbox(!parameters.compare("yes"));
                }
                else if (!order.compare("ConsentUI.IsVisible")) {
                    std::string info("status=\"" + parameters + "\"");
                    this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());

                    this->front.set_consent_ui_visible(!parameters.compare("yes"));
                }
                else if (!order.compare("Self.Status")) {
                    std::string info("status=\"" + parameters + "\"");
                    this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());

                    this->session_probe_close_pending = (parameters.compare("Closing") == 0);
                }
                else if (!order.compare("InputLanguage")) {
                    const char * subitems          = parameters.c_str();
                    const char * subitem_separator = ::strchr(subitems, '\x01');

                    if (subitem_separator) {
                        std::string code(subitems, subitem_separator - subitems);
                        std::string display_name(subitem_separator + 1);

                        std::string info("code=\"" + code + "\" name=\"" + display_name + "\"");
                        this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());

                        this->front.set_keylayout(::strtol(code.c_str(), nullptr, 16));
                    }
                    else {
                        message_format_invalid = true;
                    }
                }
                else if (!order.compare("NewProcess") ||
                         !order.compare("CompletedProcess")) {
                    std::string info("command_line=\"" + parameters + "\"");
                    this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());
                }
                else if (!order.compare("OutboundConnectionBlocked")) {
                    const char * subitems          = parameters.c_str();
                    const char * subitem_separator = ::strchr(subitems, '\x01');

                    if (subitem_separator) {
                        std::string rule(subitems, subitem_separator - subitems);
                        std::string application_name(subitem_separator + 1);

                        std::string info("rule=\"" + rule + "\" application_name=\"" + application_name + "\"");
                        this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());

                        char message[4096];
#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
# endif
                        snprintf(message, sizeof(message),
                            TR("process_interrupted_security_policies", this->lang),
                            application_name.c_str());
#ifdef __GNUG__
    #pragma GCC diagnostic pop
# endif

                        std::string string_message = message;
                        this->display_osd_message(string_message);
                    }
                    else {
                        message_format_invalid = true;
                    }
                }
                else if (!order.compare("ForegroundWindowChanged")) {
                    const char * subitems          = parameters.c_str();
                    const char * subitem_separator = ::strchr(subitems, '\x01');

                    if (subitem_separator) {
                        std::string text(subitems, subitem_separator - subitems);
                        std::string remaining(subitem_separator + 1);

                        subitems          = remaining.c_str();
                        subitem_separator = ::strchr(subitems, '\x01');

                        if (subitem_separator) {
                            std::string window_class(subitems, subitem_separator - subitems);
                            std::string command_line(subitem_separator + 1);

                            std::string info("text=\"" + text + "\" class=\"" + window_class + "\" command_line=\"" + command_line + "\"");
                            this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());
                        }
                        else {
                            message_format_invalid = true;
                        }
                    }
                    else {
                        message_format_invalid = true;
                    }
                }
                else if (!order.compare("Button.Clicked")) {
                    const char * subitems          = parameters.c_str();
                    const char * subitem_separator = ::strchr(subitems, '\x01');

                    if (subitem_separator) {
                        std::string window(subitems, subitem_separator - subitems);
                        std::string button(subitem_separator + 1);

                        std::string info("window=\"" + window + "\" button=\"" + button + "\"");
                        this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());
                    }
                    else {
                        message_format_invalid = true;
                    }
                }
                else if (!order.compare("Edit.Changed")) {
                    const char * subitems          = parameters.c_str();
                    const char * subitem_separator = ::strchr(subitems, '\x01');

                    if (subitem_separator) {
                        std::string window(subitems, subitem_separator - subitems);
                        std::string edit(subitem_separator + 1);

                        std::string info("window=\"" + window + "\" edit=\"" + edit + "\"");
                        this->acl->log4((this->verbose & 1), order.c_str(), info.c_str());
                    }
                    else {
                        message_format_invalid = true;
                    }
                }
                else {
                    LOG(LOG_WARNING,
                        "mod_rdp::process_session_probe_event: Unexpected order. Session Probe channel data=\"%s\"",
                        message);
                }

                bool contian_window_title = false;
                this->front.session_update(session_probe_channel_message.c_str(),
                    contian_window_title);
            }
            else {
                message_format_invalid = true;
            }

            if (message_format_invalid) {
                LOG(LOG_WARNING,
                    "mod_rdp::process_session_probe_event: Invalid message format. Session Probe channel data=\"%s\"",
                    message);
            }
        }
    }

    void process_cliprdr_event(
            const CHANNELS::ChannelDef & cliprdr_channel, InStream & stream,
            uint32_t length, uint32_t flags, size_t chunk_size) {
        BaseVirtualChannel& channel = this->get_clipboard_virtual_channel();

        std::unique_ptr<AsynchronousTask> out_asynchronous_task;

        channel.process_server_message(length, flags, stream.get_current(), chunk_size,
            out_asynchronous_task);

        REDASSERT(!out_asynchronous_task);
    }   // process_cliprdr_event

    void process_rail_event(const CHANNELS::ChannelDef & rail_channel,
            InStream & stream, uint32_t length, uint32_t flags, size_t chunk_size) {
        REDASSERT(stream.in_remain() == chunk_size);

        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::process_rail_event: Server RAIL PDU.");
        }

        const auto saved_stream_p = stream.get_current();

        const uint16_t orderType   = stream.in_uint16_le();
        const uint16_t orderLength = stream.in_uint16_le();

        if (this->verbose & 1) {
            LOG(LOG_INFO, "mod_rdp::process_rail_event: orderType=%u orderLength=%u.",
                orderType, orderLength);
        }

        this->send_to_front_channel(
            rail_channel.name, saved_stream_p, length, chunk_size, flags
        );
    }

    void process_rdpdr_event(const CHANNELS::ChannelDef & rdpdr_channel,
            InStream & stream, uint32_t length, uint32_t flags, size_t chunk_size) {
        if (this->authorization_channels.rdpdr_type_all_is_authorized() &&
            !this->file_system_drive_manager.HasManagedDrive()) {
            if (this->verbose && (flags & CHANNELS::CHANNEL_FLAG_LAST)) {
                LOG(LOG_INFO,
                    "mod_rdp::process_rdpdr_event: "
                        "send Chunked Virtual Channel Data transparently.");
            }

            this->send_to_front_channel(
                channel_names::rdpdr, stream.get_current(), length, chunk_size, flags);
            return;
        }

        BaseVirtualChannel& channel = this->get_file_system_virtual_channel();

        std::unique_ptr<AsynchronousTask> out_asynchronous_task;

        channel.process_server_message(length, flags, stream.get_current(), chunk_size,
            out_asynchronous_task);

        if (out_asynchronous_task) {
            if (this->asynchronous_tasks.empty()) {
                this->asynchronous_task_event.~wait_obj();
                new (&this->asynchronous_task_event) wait_obj();

                out_asynchronous_task->configure_wait_object(this->asynchronous_task_event);
            }

            this->asynchronous_tasks.push_back(std::move(out_asynchronous_task));
        }
    }
};

#endif
