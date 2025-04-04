#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

using namespace std;

typedef uint64_t u64;

enum class Action
{
    ATTACK,
    RETREAT,
    _NULL,
};

enum class Role
{
    FAITHFUL,
    TRAITOR,
    _NULL,
};

random_device rd;
mt19937       gen(rd());

void debugSetup(const u64    &nGenerals,
                const u64    &commanderID,
                vector<Role> &roles)
{
    cout << "=============  SETUP  ==============" << endl;
    cout << "commander: " << commanderID << endl;

    cout << "roles: ";
    for (u64 i = 0; i < nGenerals; i++)
        cout << (roles[i] == Role::FAITHFUL ? "Faithful" : roles[i] == Role::TRAITOR ? "Traitor"
                                                                                     : "?")
             << ", ";

    cout << endl;
    cout << "============  END SETUP  =============" << endl;
    cout << endl;
}

void debugMessages(const vector<vector<Action>> &messages, const u64 &round, const unordered_set<u64> &actingCommanders)
{
    cout << "============  MESSAGES  =============" << endl;
    cout << "round: " << round << endl;
    for (u64 i = 0; i < messages.size(); i++)
    {
        if (actingCommanders.find(i) != actingCommanders.end()) continue;

        for (u64 j = 0; j < messages[i].size(); j++)
        {
            if (actingCommanders.find(j) != actingCommanders.end()) continue;

            cout << (messages[i][j] == Action::ATTACK ? "A" : messages[i][j] == Action::RETREAT ? "R"
                                                                                                : "?")
                 << ", ";
        }
        cout << endl;
    }
    cout << "============  END MESSAGES  =============" << endl;
    cout << endl;
}

bool flipCoin()
{
    uniform_int_distribution<u64> dis(0, 1);
    return dis(gen);
}

void assignRoles(const u64    &nGenerals,
                 const u64    &nTraitors,
                 u64          &commanderID,
                 vector<Role> &roles)
{
    uniform_int_distribution<u64> dis(0, nGenerals - 1);
    commanderID = dis(gen);

    for (u64 i = 0; i < nTraitors; i++)
    {
        u64 newTraitorID = dis(gen);
        while (roles[newTraitorID] == Role::TRAITOR)
            newTraitorID = dis(gen);

        roles[newTraitorID] = Role::TRAITOR;
    }

    for (u64 i = 0; i < nGenerals; i++)
    {
        if (roles[i] == Role::_NULL)
            roles[i] = Role::FAITHFUL;
    }
}

void sendGoodOrders(const u64              &nGenerals,
                    vector<vector<Action>> &initialMessages,
                    const unordered_set<u64> &actingCommanders)
{
    Action intendedAction = flipCoin() ? Action::ATTACK : Action::RETREAT;
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (actingCommanders.find(i) != actingCommanders.end()) continue;

        initialMessages[i][i] = intendedAction;
    }
}

void sendRandomOrders(const u64              &nGenerals,
                      vector<vector<Action>> &initialMessages,
                      const unordered_set<u64> &actingCommanders)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (actingCommanders.find(i) != actingCommanders.end()) continue;

        initialMessages[i][i] = flipCoin() ? Action::ATTACK : Action::RETREAT;
    }
}

void sendInitialOrders(const u64              &nGenerals,
                       vector<vector<Action>> &initialMessages,
                       const Role             &commanderRole,
                       unordered_set<u64>     &actingCommanders)
{
    if (commanderRole == Role::FAITHFUL)
        sendGoodOrders(nGenerals,
                       initialMessages,
                       actingCommanders);
    else
        sendRandomOrders(nGenerals,
                         initialMessages,
                         actingCommanders);
}

void getInputs(u64 &nGenerals, u64 &nTraitors)
{
    cout << "Enter number of generals: ";
    cin >> nGenerals;

    cout << "Enter number of traitors: ";
    cin >> nTraitors;

    assert(nGenerals != 0 && nTraitors != 0 && nGenerals > (3 * nTraitors));
}

Action getMajority(const vector<vector<Action>> &actions, const u64 &nGenerals)
{
    vector<u64> attackMessages(nGenerals, 0);
    vector<u64> retreatMessages(nGenerals, 0);

    for (u64 i = 0; i < nGenerals; i++)
    {
        for (u64 j = 0; j < nGenerals; j++)
        {
            if (actions[i][j] == Action::ATTACK)
                attackMessages[i]++;
            else if (actions[i][j] == Action::RETREAT)
                retreatMessages[i]++;
        }
    }

    vector<Action> decisions(nGenerals, Action::_NULL);
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (attackMessages[i] > retreatMessages[i])
            decisions[i] = Action::ATTACK;
        else
            decisions[i] = Action::RETREAT;
    }

    u64 attackCount  = 0;
    u64 retreatCount = 0;
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (decisions[i] == Action::ATTACK)
            attackCount++;
        else if (decisions[i] == Action::RETREAT)
            retreatCount++;
    }

    if (attackCount > retreatCount)
        return Action::ATTACK;
    else
        return Action::RETREAT;
}

void relayMessages(const u64                    &nGenerals,
                   const unordered_set<u64>     &actingCommanders,
                   vector<vector<Action>>       &thisRoundMessages,
                   const vector<vector<Action>> &prevRoundMessages)
{
    cout << "relaying messages" << endl;
    cout << "prevRoundMessages: " << endl;
    debugMessages(prevRoundMessages, 2, actingCommanders);

    for (u64 sender = 0; sender < nGenerals; sender++)
    {
        if (actingCommanders.find(sender) != actingCommanders.end()) continue;

        for (u64 receiver = 0; receiver < nGenerals; receiver++)
        {
            if (actingCommanders.find(receiver) != actingCommanders.end()) continue;

            if (thisRoundMessages[sender][receiver] == Action::_NULL)
                thisRoundMessages[sender][receiver] = prevRoundMessages[sender][sender];
        }
    }

    cout << "thisRoundMessages: " << endl;
    debugMessages(thisRoundMessages, 2, actingCommanders);
}

Action executeRounds(u64                     nRounds,
                     u64                     nGenerals,
                     vector<Role>           &roles,
                     unordered_set<u64>     &actingCommanders,
                     vector<vector<Action>> &initialOrders,
                     const u64              &actingCommander)
{
    if (nRounds == 0)
    {
        cout << "Current commander: " << actingCommander << endl;
        vector<vector<Action>> thisRoundMessages = vector<vector<Action>>(nGenerals, vector<Action>(nGenerals, Action::_NULL));
        relayMessages(nGenerals,
                      actingCommanders,
                      thisRoundMessages,
                      initialOrders);
        return getMajority(thisRoundMessages, nGenerals);
    }

    vector<vector<Action>> thisRoundMessages = vector<vector<Action>>(nGenerals, vector<Action>(nGenerals, Action::_NULL));
    for (u64 actingCommander = 0; actingCommander < nGenerals; actingCommander++)
    {
        if (actingCommanders.find(actingCommander) != actingCommanders.end()) continue;

        actingCommanders.insert(actingCommander);

        vector<vector<Action>> initialOrders = vector<vector<Action>>(nGenerals, vector<Action>(nGenerals, Action::_NULL));
        sendInitialOrders(nGenerals,
                         initialOrders,
                         roles[actingCommander],
                         actingCommanders);

        const Action agreedAction = executeRounds(nRounds - 1,
                                                  nGenerals,
                                                  roles,
                                                  actingCommanders,
                                                  initialOrders,
                                                  actingCommander);
        thisRoundMessages[actingCommander][actingCommander] = agreedAction;

        actingCommanders.erase(actingCommander);
    }

    relayMessages(nGenerals,
                  actingCommanders,
                  thisRoundMessages,
                  thisRoundMessages);
    return getMajority(thisRoundMessages, nGenerals);
}

int main()
{
    u64 nGenerals;
    u64 nTraitors;
    getInputs(nGenerals, nTraitors);

    u64 nRounds = nTraitors;

    u64                commanderID;
    unordered_set<u64> actingCommanders;
    vector<Role>       roles(nGenerals, Role::_NULL);

    assignRoles(nGenerals, nTraitors, commanderID, roles);
    // debugSetup(nGenerals, commanderID, roles);

    const Role             commanderRole = roles[commanderID];
    vector<vector<Action>> initialOrders = vector<vector<Action>>(nGenerals, vector<Action>(nGenerals, Action::_NULL));

    actingCommanders.insert(commanderID);
    sendInitialOrders(nGenerals,
                      initialOrders,
                      commanderRole,
                      actingCommanders);
    cout << "initial orders: " << endl;
    debugMessages(initialOrders, nRounds, actingCommanders);

    const Action finalDecision = executeRounds(nRounds - 1,
                                               nGenerals,
                                               roles,
                                               actingCommanders,
                                               initialOrders,
                                               commanderID);

    cout << "final decision: " << (finalDecision == Action::ATTACK ? "A" : "R") << endl;
}
