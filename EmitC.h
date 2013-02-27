#pragma once

struct cg_Backend;
struct mem_Allocator;
struct mc_MachineCode;

struct cg_Backend* 
ec_newCCodeBackend(struct mem_Allocator* allocator, struct mc_MachineCode* mc);

