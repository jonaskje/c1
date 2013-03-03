#pragma once

struct cg_Backend;
struct mem_Allocator;
struct mc_MachineCode;

struct cg_Backend* 
x64_newBackend(struct mem_Allocator* allocator, struct mc_MachineCode* mc);

