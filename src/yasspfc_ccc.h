#ifndef YASSPFC_CCC
#define YASSPFC_CCC

#if COCOS2D_VERSION >= 0x00030000
#define YASSPFC_CC_OBJECT    cocos2d::Ref
#define YASSPFC_CC_TEXTURE2D cocos2d::Texture2D
#define YASSPFC_CC_SIZE      cocos2d::Size
#define YASSPFC_CC_NODERGBA  cocos2d::Node
#define YASSPFC_CC_MAT4      cocos2d::Mat4
#define YASSPFC_CC_COLOR3B   cocos2d::Color3B
#else
#define YASSPFC_CC_OBJECT    cocos2d::CCObject
#define YASSPFC_CC_TEXTURE2D cocos2d::CCTexture2D
#define YASSPFC_CC_SIZE      cocos2d::CCSize
#define YASSPFC_CC_NODERGBA  cocos2d::CCNodeRGBA
#define YASSPFC_CC_MAT4      kmMat4
#define YASSPFC_CC_COLOR3B   cocos2d::ccColor3B
#endif

#endif
