
#include "osapi.h"
#include "c_types.h"
#include "user_xmpp_analyzer.h"
/*
<message id="${id}" to="${to}" from="${from}" type="chat">
    <properties xmlns="http://www.jivesoftware.com/xmlns/xmpp/properties">
    </properties>
    <body>
   "${content}"
    </body>
    <delay xmlns="urn:xmpp:delay" from="${from}" stamp="${stamp}" />
    <thread>${thread}</thread>
</message>
*/

#define XMPP_MESSAGE "<message id=%s to=%s from=%s type=\"chat\">\
<properties xmlns=\"http://www.jivesoftware.com/xmlns/xmpp/properties\">\
</properties>\
<body>\
%s\
</body>\
</message>"

ICACHE_FLASH_ATTR sint8 message_make(uint8* dst, uint8* id, uint8_t* to, uint8* from, uint8* content)
{
	if(NULL == dst)
		return -1;

	os_sprintf(dst, XMPP_MESSAGE, id, to, from, content);

	return 0;
}

ICACHE_FLASH_ATTR sint8 message_draw(uint8* src, uint8* id, uint8_t* to, uint8* from, uint8* content)
{
	uint8_t* ptr0 = NULL, *ptr1 = NULL;

	if(NULL == src || \
	   /*NULL == os_strstr(src, "<message>") || \*/
	   NULL == os_strstr(src, "</message>"))
		return -1;

	if(NULL == (ptr0 = os_strstr(src, "id=")))
		return -2;
	if(NULL == (ptr1 = os_strstr(src, "to=")))
		return -3;
	if(NULL != id)
		os_memcpy(id, ptr0+3, ptr1 - ptr0 - 4);

	if(NULL == (ptr0 = os_strstr(src, "from=")))
		return -4;
	if(NULL != to)
		os_memcpy(to, ptr1+3, ptr0 - ptr1 - 4);

	if(NULL == (ptr1 = os_strstr(src, "type=")))
		return -5;
	if(NULL != from)
		os_memcpy(from, ptr0+5, ptr1 - ptr0 - 6);

	if(NULL == (ptr0 = os_strstr(src, "<body>")))
		return -6;
	if(NULL == (ptr1 = os_strstr(src, "</body>")))
		return -7;
	if(NULL != content)
		os_memcpy(content, ptr0+6, ptr1 - ptr0 - 6);
}

