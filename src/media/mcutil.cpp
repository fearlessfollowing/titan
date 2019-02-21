
#include <binder/ProcessState.h>
#include <media/stagefright/foundation/base64.h>
#include <media/stagefright/foundation/AString.h>
#include "mcutil.h"

void mcutil::threadpool_init()
{
	android::ProcessState::self()->startThreadPool();
}

android::sp<android::ABuffer> mcutil::base64_decode(const char* s)
{
	return android::decodeBase64(android::AString(s));
}

std::string mcutil::base64_encode(const unsigned char* data, unsigned int size)
{
	android::AString out;
	android::encodeBase64(data, size, &out);
	return std::string(out.c_str());
}