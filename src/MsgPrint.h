#ifndef __MSGPRINT_H__
#define __MSGPRINT_H__

class MsgPrint {
public:
	enum SEVERE {ERR, WARN, INFO};
	static void msgPrint(SEVERE s, const char *msg);
};

#endif
