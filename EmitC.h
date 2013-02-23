#pragma once

struct cg_Backend;
struct mem_Allocator;

struct cg_Backend* 
ec_newCCodeBackend(struct mem_Allocator* allocator);

