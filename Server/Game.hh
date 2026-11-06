#pragma once

#include <Server/TeamManager.hh>

#include <Shared/Simulation.hh>

#include <set>
#include <vector>

class Client;

class GameInstance {
    std::set<Client *> clients;
    TeamManager team_manager;
public:
    Simulation simulation;
    uint8_t gamemode;
    GameInstance(uint8_t);
    // the signed-in players that are in the world (for the master's crash backup)
    void collect_backup(std::vector<struct BackupPlayer> &);
    void init();
    void tick();
    void add_client(Client *, EntityID);
    void remove_client(Client *);
    // The progress saves of an account only exist as the entities of its camera
    // (and in the master). save_progress sends a new one to the master,
    // apply_progress puts a restored one into the camera and its inventory and
    // returns the score, drop_progress removes the entity of one, and
    // show_progress_saves makes entities of the list the master sent.
    void save_progress(Client *);
    uint32_t apply_progress(Client *, struct PlayerProgress const &);
    void drop_progress(Client *, uint64_t restore_id);
    void show_progress_saves(Client *, std::vector<struct PlayerProgress> const &);
    // the saves a camera shows (for a client that moves to the other game)
    std::vector<struct PlayerProgress> progress_of(Client *);
};