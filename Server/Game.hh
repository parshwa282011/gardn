#pragma once

#include <Server/TeamManager.hh>

#include <Shared/Simulation.hh>

#include <set>

class Client;

class GameInstance {
    std::set<Client *> clients;
    TeamManager team_manager;
public:
    Simulation simulation;
    uint8_t gamemode;
    GameInstance(uint8_t);
    void init();
    void tick();
    void add_client(Client *, EntityID);
    void remove_client(Client *);
    void save_progress(Client *);
    uint32_t restore_progress(Client *, uint64_t);
    void find_progress_saves(Client *);
};