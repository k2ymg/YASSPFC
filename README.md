# YASSPFC


## how to use

```c++
#include "yasspfc.h"

{
    yasspfc::SSBP* data = yasspfc::SSBP::create("sample.ssbp");

    auto player = yasspfc::SSPlayer::create(data);
    player->set_anime("anime0", "anime_1");
    this->addChild(player);
    player->play(true);
}
```
