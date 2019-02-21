
#include "ins_uuid.h"
#include "uuid/uuid.h"

std::string ins_uuid_gen()
{
	uuid_t uuid_b;
	uuid_generate(uuid_b);

	char uuid_s[33] = {0};
	for (int i = 0; i < 16; i++)
	{
		sprintf(uuid_s+i*2, "%02x", uuid_b[i]);
	}

    return std::string(uuid_s);
}