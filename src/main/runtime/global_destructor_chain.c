/* sc: compiler=mwcps2-3.0b52-030722; flags=-O4,p -sdatathreshold 0 -inline auto,deferred,bottomup,level=5 -enum min -Cpp_exceptions on */
/* global_destructor_chain.c -- Metrowerks' C++ runtime: every static object with a destructor is
 * registered on a chain at its construction (MAIN 0x182810). The file name and body are the
 * PowerPC runtime's (PowerPC_EABI_Support/Runtime/Src/global_destructor_chain.c); SOCOM 1 links
 * __destroy_global_chain just before it (0x142B70: reverse definition order), SOCOM II strips it.
 * -sdatathreshold 0 is needed: at the default the chain's head goes to $gp small data (8 words
 * off). */

typedef struct DestructorChain {
    struct DestructorChain *next;
    void *destructor;
    void *object;
} DestructorChain;

DestructorChain *__global_destructor_chain;     /* 0x1D5948 */

void *__register_global_object(void *object, void *destructor, void *regmem)
{
    ((DestructorChain *)regmem)->next = __global_destructor_chain;
    ((DestructorChain *)regmem)->destructor = destructor;
    ((DestructorChain *)regmem)->object = object;
    __global_destructor_chain = (DestructorChain *)regmem;
    return object;
}
