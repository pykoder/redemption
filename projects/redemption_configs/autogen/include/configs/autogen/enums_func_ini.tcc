//
// DO NOT EDIT THIS FILE BY HAND -- YOUR CHANGES WILL BE OVERWRITTEN
//

namespace
{

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<CaptureFlags> /*type*/,
    CaptureFlags x
){
    static_assert(sizeof(CaptureFlags) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(CaptureFlags & x, ::configs::spec_type<CaptureFlags> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<CaptureFlags>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 15>()
    )) {
        return err;
    }

    x = static_cast<CaptureFlags>(xi);
    return no_parse_error;
}

namespace
{
    inline constexpr zstring_view enum_zstr_Level[] {
        "low"_zv,
        "medium"_zv,
        "high"_zv,
    };

    inline constexpr std::pair<chars_view, Level> enum_str_value_Level[] {
        {"LOW"_av, Level::low},
        {"MEDIUM"_av, Level::medium},
        {"HIGH"_av, Level::high},
    };
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<Level> /*type*/,
    Level x
){
    (void)zbuf;
    assert(is_valid_enum_value(x));
    return enum_zstr_Level[static_cast<unsigned long>(x)];
}

parse_error parse_from_cfg(Level & x, ::configs::spec_type<Level> /*type*/, chars_view value)
{
    return parse_str_value_pairs<enum_str_value_Level>(
        x, value, "bad value, expected: low, medium, high");
}

namespace
{
    inline constexpr zstring_view enum_zstr_Language[] {
        "en"_zv,
        "fr"_zv,
    };

    inline constexpr std::pair<chars_view, Language> enum_str_value_Language[] {
        {"EN"_av, Language::en},
        {"FR"_av, Language::fr},
    };
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<Language> /*type*/,
    Language x
){
    (void)zbuf;
    assert(is_valid_enum_value(x));
    return enum_zstr_Language[static_cast<unsigned long>(x)];
}

parse_error parse_from_cfg(Language & x, ::configs::spec_type<Language> /*type*/, chars_view value)
{
    return parse_str_value_pairs<enum_str_value_Language>(
        x, value, "bad value, expected: en, fr");
}

namespace
{
    inline constexpr zstring_view enum_zstr_ClipboardEncodingType[] {
        "utf8"_zv,
        "latin1"_zv,
    };

    inline constexpr std::pair<chars_view, ClipboardEncodingType> enum_str_value_ClipboardEncodingType[] {
        {"UTF-8"_av, ClipboardEncodingType::utf8},
        {"LATIN1"_av, ClipboardEncodingType::latin1},
    };
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<ClipboardEncodingType> /*type*/,
    ClipboardEncodingType x
){
    (void)zbuf;
    assert(is_valid_enum_value(x));
    return enum_zstr_ClipboardEncodingType[static_cast<unsigned long>(x)];
}

parse_error parse_from_cfg(ClipboardEncodingType & x, ::configs::spec_type<ClipboardEncodingType> /*type*/, chars_view value)
{
    return parse_str_value_pairs<enum_str_value_ClipboardEncodingType>(
        x, value, "bad value, expected: utf-8, latin1");
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<KeyboardLogFlagsCP> /*type*/,
    KeyboardLogFlagsCP x
){
    static_assert(sizeof(KeyboardLogFlagsCP) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(KeyboardLogFlagsCP & x, ::configs::spec_type<KeyboardLogFlagsCP> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<KeyboardLogFlagsCP>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 3>()
    )) {
        return err;
    }

    x = static_cast<KeyboardLogFlagsCP>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<KeyboardLogFlags> /*type*/,
    KeyboardLogFlags x
){
    static_assert(sizeof(KeyboardLogFlags) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(KeyboardLogFlags & x, ::configs::spec_type<KeyboardLogFlags> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<KeyboardLogFlags>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 7>()
    )) {
        return err;
    }

    x = static_cast<KeyboardLogFlags>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<ClipboardLogFlags> /*type*/,
    ClipboardLogFlags x
){
    static_assert(sizeof(ClipboardLogFlags) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(ClipboardLogFlags & x, ::configs::spec_type<ClipboardLogFlags> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<ClipboardLogFlags>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 7>()
    )) {
        return err;
    }

    x = static_cast<ClipboardLogFlags>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<FileSystemLogFlags> /*type*/,
    FileSystemLogFlags x
){
    static_assert(sizeof(FileSystemLogFlags) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(FileSystemLogFlags & x, ::configs::spec_type<FileSystemLogFlags> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<FileSystemLogFlags>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 7>()
    )) {
        return err;
    }

    x = static_cast<FileSystemLogFlags>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<ServerNotification> /*type*/,
    ServerNotification x
){
    static_assert(sizeof(ServerNotification) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(ServerNotification & x, ::configs::spec_type<ServerNotification> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<ServerNotification>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 7>()
    )) {
        return err;
    }

    x = static_cast<ServerNotification>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<ServerCertCheck> /*type*/,
    ServerCertCheck x
){
    static_assert(sizeof(ServerCertCheck) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(ServerCertCheck & x, ::configs::spec_type<ServerCertCheck> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<ServerCertCheck>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 3>()
    )) {
        return err;
    }

    x = static_cast<ServerCertCheck>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<TraceType> /*type*/,
    TraceType x
){
    static_assert(sizeof(TraceType) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(TraceType & x, ::configs::spec_type<TraceType> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<TraceType>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<TraceType>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<KeyboardInputMaskingLevel> /*type*/,
    KeyboardInputMaskingLevel x
){
    static_assert(sizeof(KeyboardInputMaskingLevel) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(KeyboardInputMaskingLevel & x, ::configs::spec_type<KeyboardInputMaskingLevel> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<KeyboardInputMaskingLevel>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 3>()
    )) {
        return err;
    }

    x = static_cast<KeyboardInputMaskingLevel>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<SessionProbeOnLaunchFailure> /*type*/,
    SessionProbeOnLaunchFailure x
){
    static_assert(sizeof(SessionProbeOnLaunchFailure) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(SessionProbeOnLaunchFailure & x, ::configs::spec_type<SessionProbeOnLaunchFailure> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<SessionProbeOnLaunchFailure>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<SessionProbeOnLaunchFailure>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<VncBogusClipboardInfiniteLoop> /*type*/,
    VncBogusClipboardInfiniteLoop x
){
    static_assert(sizeof(VncBogusClipboardInfiniteLoop) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(VncBogusClipboardInfiniteLoop & x, ::configs::spec_type<VncBogusClipboardInfiniteLoop> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<VncBogusClipboardInfiniteLoop>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<VncBogusClipboardInfiniteLoop>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<ColorDepthSelectionStrategy> /*type*/,
    ColorDepthSelectionStrategy x
){
    static_assert(sizeof(ColorDepthSelectionStrategy) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(ColorDepthSelectionStrategy & x, ::configs::spec_type<ColorDepthSelectionStrategy> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<ColorDepthSelectionStrategy>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 1>()
    )) {
        return err;
    }

    x = static_cast<ColorDepthSelectionStrategy>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<WrmCompressionAlgorithm> /*type*/,
    WrmCompressionAlgorithm x
){
    static_assert(sizeof(WrmCompressionAlgorithm) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(WrmCompressionAlgorithm & x, ::configs::spec_type<WrmCompressionAlgorithm> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<WrmCompressionAlgorithm>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<WrmCompressionAlgorithm>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<RdpCompression> /*type*/,
    RdpCompression x
){
    static_assert(sizeof(RdpCompression) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(RdpCompression & x, ::configs::spec_type<RdpCompression> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<RdpCompression>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 4>()
    )) {
        return err;
    }

    x = static_cast<RdpCompression>(xi);
    return no_parse_error;
}

namespace
{
    inline constexpr zstring_view enum_zstr_OcrLocale[] {
        "latin"_zv,
        "cyrillic"_zv,
    };

    inline constexpr std::pair<chars_view, OcrLocale> enum_str_value_OcrLocale[] {
        {"LATIN"_av, OcrLocale::latin},
        {"CYRILLIC"_av, OcrLocale::cyrillic},
    };
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<OcrLocale> /*type*/,
    OcrLocale x
){
    (void)zbuf;
    assert(is_valid_enum_value(x));
    return enum_zstr_OcrLocale[static_cast<unsigned long>(x)];
}

parse_error parse_from_cfg(OcrLocale & x, ::configs::spec_type<OcrLocale> /*type*/, chars_view value)
{
    return parse_str_value_pairs<enum_str_value_OcrLocale>(
        x, value, "bad value, expected: latin, cyrillic");
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<BogusNumberOfFastpathInputEvent> /*type*/,
    BogusNumberOfFastpathInputEvent x
){
    static_assert(sizeof(BogusNumberOfFastpathInputEvent) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(BogusNumberOfFastpathInputEvent & x, ::configs::spec_type<BogusNumberOfFastpathInputEvent> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<BogusNumberOfFastpathInputEvent>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<BogusNumberOfFastpathInputEvent>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<SessionProbeOnKeepaliveTimeout> /*type*/,
    SessionProbeOnKeepaliveTimeout x
){
    static_assert(sizeof(SessionProbeOnKeepaliveTimeout) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(SessionProbeOnKeepaliveTimeout & x, ::configs::spec_type<SessionProbeOnKeepaliveTimeout> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<SessionProbeOnKeepaliveTimeout>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<SessionProbeOnKeepaliveTimeout>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<SmartVideoCropping> /*type*/,
    SmartVideoCropping x
){
    static_assert(sizeof(SmartVideoCropping) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(SmartVideoCropping & x, ::configs::spec_type<SmartVideoCropping> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<SmartVideoCropping>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<SmartVideoCropping>(xi);
    return no_parse_error;
}

namespace
{
    inline constexpr zstring_view enum_zstr_RdpModeConsole[] {
        "allow"_zv,
        "force"_zv,
        "forbid"_zv,
    };

    inline constexpr std::pair<chars_view, RdpModeConsole> enum_str_value_RdpModeConsole[] {
        {"ALLOW"_av, RdpModeConsole::allow},
        {"FORCE"_av, RdpModeConsole::force},
        {"FORBID"_av, RdpModeConsole::forbid},
    };
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<RdpModeConsole> /*type*/,
    RdpModeConsole x
){
    (void)zbuf;
    assert(is_valid_enum_value(x));
    return enum_zstr_RdpModeConsole[static_cast<unsigned long>(x)];
}

parse_error parse_from_cfg(RdpModeConsole & x, ::configs::spec_type<RdpModeConsole> /*type*/, chars_view value)
{
    return parse_str_value_pairs<enum_str_value_RdpModeConsole>(
        x, value, "bad value, expected: allow, force, forbid");
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<SessionProbeDisabledFeature> /*type*/,
    SessionProbeDisabledFeature x
){
    static_assert(sizeof(SessionProbeDisabledFeature) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(SessionProbeDisabledFeature & x, ::configs::spec_type<SessionProbeDisabledFeature> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<SessionProbeDisabledFeature>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 511>()
    )) {
        return err;
    }

    x = static_cast<SessionProbeDisabledFeature>(xi);
    return no_parse_error;
}

namespace
{
    inline constexpr zstring_view enum_zstr_RdpStoreFile[] {
        "never"_zv,
        "always"_zv,
        "on_invalid_verification"_zv,
    };

    inline constexpr std::pair<chars_view, RdpStoreFile> enum_str_value_RdpStoreFile[] {
        {"NEVER"_av, RdpStoreFile::never},
        {"ALWAYS"_av, RdpStoreFile::always},
        {"ON_INVALID_VERIFICATION"_av, RdpStoreFile::on_invalid_verification},
    };
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<RdpStoreFile> /*type*/,
    RdpStoreFile x
){
    (void)zbuf;
    assert(is_valid_enum_value(x));
    return enum_zstr_RdpStoreFile[static_cast<unsigned long>(x)];
}

parse_error parse_from_cfg(RdpStoreFile & x, ::configs::spec_type<RdpStoreFile> /*type*/, chars_view value)
{
    return parse_str_value_pairs<enum_str_value_RdpStoreFile>(
        x, value, "bad value, expected: never, always, on_invalid_verification");
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<SessionProbeOnAccountManipulation> /*type*/,
    SessionProbeOnAccountManipulation x
){
    static_assert(sizeof(SessionProbeOnAccountManipulation) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(SessionProbeOnAccountManipulation & x, ::configs::spec_type<SessionProbeOnAccountManipulation> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<SessionProbeOnAccountManipulation>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<SessionProbeOnAccountManipulation>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<ClientAddressSent> /*type*/,
    ClientAddressSent x
){
    static_assert(sizeof(ClientAddressSent) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(ClientAddressSent & x, ::configs::spec_type<ClientAddressSent> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<ClientAddressSent>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        zero_integral<ul>(),
        std::integral_constant<ul, 2>()
    )) {
        return err;
    }

    x = static_cast<ClientAddressSent>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<ColorDepth> /*type*/,
    ColorDepth x
){
    static_assert(sizeof(ColorDepth) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(ColorDepth & x, ::configs::spec_type<ColorDepth> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<ColorDepth>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        min_integral<ul>(),
        max_integral<ul>()
    )) {
        return err;
    }

    switch (xi) {
        case ul(ColorDepth::depth8): break;
        case ul(ColorDepth::depth15): break;
        case ul(ColorDepth::depth16): break;
        case ul(ColorDepth::depth24): break;
        case ul(ColorDepth::depth32): break;
        default: return parse_error{"unknown value"};
    }

    x = static_cast<ColorDepth>(xi);
    return no_parse_error;
}

zstring_view assign_zbuf_from_cfg(
    writable_chars_view zbuf,
    cfg_s_type<OcrVersion> /*type*/,
    OcrVersion x
){
    static_assert(sizeof(OcrVersion) <= sizeof(unsigned long));
    int sz = snprintf(zbuf.data(), zbuf.size(), "%lu", static_cast<unsigned long>(x));
    return zstring_view(zstring_view::is_zero_terminated{}, zbuf.data(), sz);
}

parse_error parse_from_cfg(OcrVersion & x, ::configs::spec_type<OcrVersion> /*type*/, chars_view value)
{
    using ul = unsigned long;
    using enum_int = std::underlying_type_t<OcrVersion>;
    static_assert(min_integral<ul>::value <= min_integral<enum_int>::value);
    static_assert(max_integral<ul>::value >= max_integral<enum_int>::value);

    ul xi = 0;
    if (parse_error err = parse_integral(
        xi, value,
        min_integral<ul>(),
        max_integral<ul>()
    )) {
        return err;
    }

    switch (xi) {
        case ul(OcrVersion::v1): break;
        case ul(OcrVersion::v2): break;
        default: return parse_error{"unknown value"};
    }

    x = static_cast<OcrVersion>(xi);
    return no_parse_error;
}

} // anonymous namespace
