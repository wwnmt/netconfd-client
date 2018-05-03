#ifndef _NETCONFD_MESSAGES_H__
#define _NETCONFD_MESSAGES_H__

#define XML_NETCONF_BASE_1_0_END "]]>]]>"

#define XML_NETCONF_HELLO_C \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
"<hello xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">" \
 "<capabilities>" \
  "<capability>urn:ietf:params:netconf:base:1.0</capability>" \
 "</capabilities>" \
"</hello>"

#endif /* _NETCONFD_MESSAGES_H__ */