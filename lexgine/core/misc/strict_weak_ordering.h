#ifndef LEXGINE_MISC_WEAK_ORDERING_H
#define LEXGINE_MISC_WEAK_ORDERING_H

#define SWO_STEP(a, op, b) \
if(a op b) return true; \
if(b op a) return false

#define SWO_END(a, op, b) return a op b;

#endif
