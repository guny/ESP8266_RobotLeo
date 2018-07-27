
#ifndef __USER_XMPP_ANALYZER_H__
#define __USER_XMPP_ANALYZER_H__

ICACHE_FLASH_ATTR sint8 message_make(uint8* dst, uint8* id, uint8_t* to, uint8* from, uint8* content);
ICACHE_FLASH_ATTR sint8 message_draw(uint8* src, uint8* id, uint8_t* to, uint8* from, uint8* content);

#endif //__USER_XMPP_ANALYZER_H__