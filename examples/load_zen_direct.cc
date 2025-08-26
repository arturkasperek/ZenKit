// Copyright © 2022-2023 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#include "zenkit/World.hh"
#include "zenkit/Stream.hh"

#include <iostream>

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <path-to-zen-file>\n";
		std::cerr << "Example: " << argv[0] << " NEWWORLD.ZEN\n";
		return -1;
	}

	try {
		// Load ZEN file directly from disk without VFS
		auto reader = zenkit::Read::from(argv[1]);
		
		zenkit::World world;
		world.load(reader.get());
		
		std::cout << "Successfully loaded world: " << argv[1] << "\n";
		std::cout << "World contains:\n";
		std::cout << "  - VOBs: " << world.world_vobs.size() << "\n";
		std::cout << "  - Mesh vertices: " << world.world_mesh.vertices.size() << "\n";
		std::cout << "  - BSP tree nodes: " << world.world_bsp_tree.nodes.size() << "\n";
		
		// If it's a save game, show additional info
		if (!world.npcs.empty()) {
			std::cout << "  - NPCs: " << world.npcs.size() << "\n";
			std::cout << "  - NPC spawn locations: " << world.npc_spawns.size() << "\n";
		}
		
	} catch (const std::exception& e) {
		std::cerr << "Error loading world: " << e.what() << "\n";
		return -1;
	}
	
	return 0;
}
