#include "CompiledCode.h"
#include "defs.h"

@implementation CompiledCode
- (id) init
{
    self = [super init];
    constants = [Array new];
    instructions = [Array new];
    return self;
}
    
- (void) addInstruction: (Instruction) inst
{
    [inst offset: [instructions count]];
    if ([inst opcode] != LABEL) {
            [instructions addItem: inst];
    }
}

- (integer) addConstant: (SchemeObject) c
{
    local integer number = [constants count];
    [constants addItem: c];
    return number;
}            
    
- (void) compile
{
    local integer index;
    local Instruction inst;
    literals = [Frame newWithSize: [constants count] link: NIL];
    code = obj_malloc (@sizeof(instruction_t) * [instructions count]);
    for (index = 0; index < [constants count]; index++) {
            [literals set: index to: (SchemeObject) [constants getItemAt: index]];
    }
    for (index = 0; index < [instructions count]; index++) {
            inst = [instructions getItemAt: index];
            [inst emitStruct: code];
    }
    [instructions makeObjectsPerformSelector: @selector(retain)];
    [instructions release];
    [constants makeObjectsPerformSelector: @selector(retain)];
    [constants release];
}

- (instruction_t []) code
{
    return code;
}

- (Frame) literals
{
    return literals;
}

@end
