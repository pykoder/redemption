/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2013
    Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan
*/

#ifndef _REDEMPTION_CORE_RDP_NLA_NTLM_NTLMMESSAGECHALLENGE_HPP_
#define _REDEMPTION_CORE_RDP_NLA_NTLM_NTLMMESSAGECHALLENGE_HPP_

#include "RDP/nla/ntlm/ntlm_message.hpp"
#include "RDP/nla/ntlm/ntlm_avpair.hpp"

// [MS-NLMP]

// 2.2.1.2   CHALLENGE_MESSAGE
// ======================================================================
// The CHALLENGE_MESSAGE defines an NTLM challenge message that is sent from
//  the server to the client. The CHALLENGE_MESSAGE is used by the server to
//  challenge the client to prove its identity.
//  For connection-oriented requests, the CHALLENGE_MESSAGE generated by the
//  server is in response to the NEGOTIATE_MESSAGE (section 2.2.1.1) from the client.

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           Signature                           |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
// |                          MessageType                          |
// +---------------+---------------+---------------+---------------+
// |                        TargetNameFields                       |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
// |                         NegotiateFlags                        |
// +---------------+---------------+---------------+---------------+
// |                         ServerChallenge                       |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
// |                           Reserved                            |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
// |                        TargetInfoFields                       |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
// |                            Version                            |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
// |                       Payload (Variable)                      |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+

// Signature (8 bytes):  An 8-byte character array that MUST contain the
//  ASCII string ('N', 'T', 'L', 'M', 'S', 'S', 'P', '\0').

// MessageType (4 bytes):  A 32-bit unsigned integer that indicates the message
//  type. This field MUST be set to 0x00000002.

// TargetNameFields (8 bytes):  If the NTLMSSP_REQUEST_TARGET flag is not set in
//  NegotiateFlags, indicating that no TargetName is required:
//  - TargetNameLen and TargetNameMaxLen SHOULD be set to zero on transmission.
//  - TargetNameBufferOffset field SHOULD be set to the offset from the beginning of the
//     CHALLENGE_MESSAGE to where the TargetName would be in Payload if it were present.
//  - TargetNameLen, TargetNameMaxLen, and TargetNameBufferOffset MUST be ignored
//     on receipt.
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |        TargetNameLen          |       TargetNameMaxLen        |
// +---------------+---------------+---------------+---------------+
// |                     TargetNameBufferOffset                    |
// +---------------+---------------+---------------+---------------+
//    TargetNameLen (2 bytes):  A 16-bit unsigned integer that defines the size,
//     in bytes, of TargetName in Payload.
//    TargetNameMaxLen (2 bytes):  A 16-bit unsigned integer that SHOULD be set
//     to the value of TargetNameLen and MUST be ignored on receipt.
//    TargetNameBufferOffset (4 bytes):  A 32-bit unsigned integer that defines
//     the offset, in bytes, from the beginning of the CHALLENGE_MESSAGE to
//     TargetName in Payload.
//     If TargetName is a Unicode string, the values of TargetNameBufferOffset and
//     TargetNameLen MUST be multiples of 2.

// NegotiateFlags (4 bytes):  A NEGOTIATE structure that contains a set of bit flags,
//  as defined in section 2.2.2.5. The server sets flags to indicate options it supports or,
//  if thre has been a NEGOTIATE_MESSAGE (section 2.2.1.1), the choices it has made from
//   the options offered by the client.

// ServerChallenge (8 bytes):  A 64-bit value that contains the NTLM challenge.
//  The challenge is a 64-bit nonce. The processing of the ServerChallenge is specified
//  in sections 3.1.5 and 3.2.5.
// Reserved (8 bytes):  An 8-byte array whose elements MUST be zero when sent and MUST be
//  ignored on receipt.

// TargetInfoFields (8 bytes):  If the NTLMSSP_NEGOTIATE_TARGET_INFO flag of
//  NegotiateFlags is clear, indicating that no TargetInfo is required:
//  - TargetInfoLen and TargetInfoMaxLen SHOULD be set to zero on transmission.
//  - TargetInfoBufferOffset field SHOULD be set to the offset from the beginning of the
//    CHALLENGE_MESSAGE to where the TargetInfo would be in Payload if it were present.
//  - TargetInfoLen, TargetInfoMaxLen, and TargetInfoBufferOffset MUST be ignored on receipt.
//  Otherwise, these fields are defined as:
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |        TargetInfoLen          |       TargetInfoMaxLen        |
// +---------------+---------------+---------------+---------------+
// |                     TargetInfoBufferOffset                    |
// +---------------+---------------+---------------+---------------+
//    TargetInfoLen (2 bytes):  A 16-bit unsigned integer that defines the size,
//     in bytes, of TargetInfo in Payload.
//    TargetInfoMaxLen (2 bytes):  A 16-bit unsigned integer that SHOULD be set
//     to the value of TargetInfoLen and MUST be ignored on receipt.
//    TargetInfoBufferOffset (4 bytes):  A 32-bit unsigned integer that defines
//     the offset, in bytes, from the beginning of the CHALLENGE_MESSAGE to
//     TargetInfo in Payload.

// Version (8 bytes):  A VERSION structure (as defined in section 2.2.2.10) that
//  is present only when the NTLMSSP_NEGOTIATE_VERSION flag is set in the
//  NegotiateFlags field. This structure is used for debugging purposes only.
//  In normal (non-debugging) protocol messages, it is ignored and does not affect
//  the NTLM message processing.


// Payload (variable):  A byte-array that contains the data referred to by the
//   TargetNameBufferOffset and TargetInfoBufferOffset message fields. Payload data
//   can be present in any order within the Payload field, with variable-length padding
//   before or after the data. The data that can be present in the Payload field of
//   this message, in no particular order, are:
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | | | | | | | | | | |1| | | | | | | | | |2| | | | | | | | | |3| |
// |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      TargetName (Variable)                    |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
// |                      TargetInfo (Variable)                    |
// +---------------+---------------+---------------+---------------+
// |                              ...                              |
// +---------------+---------------+---------------+---------------+
//    TargetName (variable):  If TargetNameLen does not equal 0x0000, TargetName
//     MUST be a byte-array that contains the name of the server authentication realm,
//     and MUST be expressed in the negotiated character set. A server that is a
//     member of a domain returns the domain of which it is a member, and a server
//     that is not a member of a domain returns the server name.
//    TargetInfo (variable):  If TargetInfoLen does not equal 0x0000, TargetInfo
//     MUST be a byte array that contains a sequence of AV_PAIR structures.
//     The AV_PAIR structure is defined in section 2.2.2.1. The length of each
//     AV_PAIR is determined by its AvLen field (plus 4 bytes).

//     Note  An AV_PAIR structure can start on any byte alignment and the sequence of
//      AV_PAIRs has no padding between structures.
//     The sequence MUST be terminated by an AV_PAIR structure with an AvId field of
//      MsvAvEOL. The total length of the TargetInfo byte array is the sum of the lengths,
//      in bytes, of the AV_PAIR structures it contains.
//     Note  If a TargetInfo AV_PAIR Value is textual, it MUST be encoded in Unicode
//      irrespective of what character set was negotiated (section 2.2.2.1).


struct NTLMChallengeMessage : public NTLMMessage {

    NtlmField TargetName;          /* 8 Bytes */
    NtlmNegotiateFlags negoFlags;  /* 4 Bytes */
    uint8_t serverChallenge[8];    /* 8 Bytes */
    // uint64_t serverChallenge;
    /* 8 Bytes reserved */
    NtlmField TargetInfo;          /* 8 Bytes */
    NtlmVersion version;           /* 8 Bytes */
    uint32_t PayloadOffset;

    NtlmAvPairList AvPairList;


    NTLMChallengeMessage()
        : NTLMMessage(NtlmChallenge)
        , PayloadOffset(12+8+4+8+8+8+8)
    {
    }

    virtual ~NTLMChallengeMessage() {}

    void emit(Stream & stream) {
        this->TargetInfo.Buffer.reset();
        this->AvPairList.emit(this->TargetInfo.Buffer);

        uint32_t currentOffset = this->PayloadOffset;
        if (this->negoFlags.flags & NTLMSSP_NEGOTIATE_VERSION) {
            this->version.ntlm_get_version_info();
        }
        else {
            currentOffset -= 8;
        }
        NTLMMessage::emit(stream);
        currentOffset += this->TargetName.emit(stream, currentOffset);
        this->negoFlags.emit(stream);
        stream.out_copy_bytes(this->serverChallenge, 8);
        stream.out_clear_bytes(8);
        currentOffset += this->TargetInfo.emit(stream, currentOffset);
        if (this->negoFlags.flags & NTLMSSP_NEGOTIATE_VERSION) {
            this->version.emit(stream);
        }
        // PAYLOAD
        this->TargetName.write_payload(stream);
        this->TargetInfo.write_payload(stream);
        stream.mark_end();
    }

    void recv(Stream & stream) {
        uint8_t * pBegin = stream.p;
        bool res;
        res = NTLMMessage::recv(stream);
        if (!res) {
            LOG(LOG_ERR, "INVALID MSG RECEIVED type: %u", this->msgType);
        }
        this->TargetName.recv(stream);
        this->negoFlags.recv(stream);
        stream.in_copy_bytes(this->serverChallenge, 8);
        // this->serverChallenge = stream.in_uint64_le();
        stream.in_skip_bytes(8);
        this->TargetInfo.recv(stream);
        if (this->negoFlags.flags & NTLMSSP_NEGOTIATE_VERSION) {
            this->version.recv(stream);
        }
        // PAYLOAD
        this->TargetName.read_payload(stream, pBegin);
        this->TargetInfo.read_payload(stream, pBegin);
        this->AvPairList.recv(this->TargetInfo.Buffer);
    }

    void avpair_decode() {
        this->TargetInfo.Buffer.reset();
        this->AvPairList.emit(this->TargetInfo.Buffer);
    }

};



#endif
