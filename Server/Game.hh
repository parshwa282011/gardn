#pragma once

#include <Server/TeamManager.hh>

#include <Shared/Simulation.hh>

#include <array>
#include <set>
#include <string>
#include <vector>

class Client;

class GameInstance {
    std::set<Client *> clients;
    TeamManager team_manager;
    // The squads of the sandbox. A squad exists while it has a code; its team entity
    // (made once in init) is what the flowers of its members have as their team.
    struct Squad {
        EntityID team;
        uint8_t color = 0;
        std::string code;
    };
    std::array<Squad, MAX_SQUADS> squads;
    void squad_set(Client *, EntityID team, uint8_t color, std::string const &code, uint8_t result);
    void squad_cleanup();
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
    // The squads (sandbox only): a player is in one, or in the normal group. Each tells
    // the player how it went (SquadResult)
    void squad_create(Client *);
    void squad_join(Client *, std::string const &code);
    void squad_leave(Client *);
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