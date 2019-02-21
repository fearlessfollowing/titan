
#include "ins_base64.h"

extern "C"
{
#include <b64/cencode.h>
#include <b64/cdecode.h>
}

std::shared_ptr<insbuff> ins_base64::decode(const std::string& in)
{
    base64_decodestate s;
    base64_init_decodestate(&s);

    auto out = std::make_shared<insbuff>(in.size()*2);
    auto size = base64_decode_block(in.c_str(), in.size(), (char*)out->data(), &s);
    out->set_offset(size);

    return out;
}

std::string ins_base64::encode(std::shared_ptr<insbuff>& in)
{
    base64_encodestate s;
    base64_init_encodestate(&s);
    
    auto out = std::make_shared<insbuff>(in->size()*2);
    char*p = (char*)out->data();
    auto ret = base64_encode_block((char*)in->data(), in->size(), p, &s);
    p += ret;

    ret = base64_encode_blockend(p, &s);
    p += ret;
    *p = 0;

    //printf("encode out:%s size:%lu\n", (char*)out->data(), strlen((char*)out->data()));

    return std::string((char*)out->data());
}