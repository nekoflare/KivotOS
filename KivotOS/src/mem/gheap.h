//
// Created by neko on 6/8/25.
//

#ifndef GHEAP_H
#define GHEAP_H

#define ALIGN_UP(x, align)   (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

#define HEAP_SIZE (16 * 1024 * 1024 * 1024ULL) // 16GB

void gheap_init();

#endif //GHEAP_H
