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

random_device                 rd;
mt19937                       gen(rd());
uniform_int_distribution<u64> coinFlipDist(0, 1);

void debugSetup(const u64    &nGenerals,
                const u64    &commanderID,
                vector<Role> &roles)
{
    cout << "commander: " << commanderID << endl;

    cout << "roles: ";
    for (u64 i = 0; i < nGenerals; i++)
        cout << (roles[i] == Role::FAITHFUL ? "Faithful" : roles[i] == Role::TRAITOR ? "Traitor"
                                                                                     : "?")
             << ", ";

    cout << endl;
    cout << endl;
}

void debugMessages(const vector<vector<Action>> &messages, const unordered_set<u64> &actingCommanders)
{
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
    cout << endl;
}

inline bool flipCoin()
{
    return coinFlipDist(gen);
}

inline Action getRandomMessage()
{
    return flipCoin() ? Action::ATTACK : Action::RETREAT;
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

void sendGoodOrders(const u64                &nGenerals,
                    vector<vector<Action>>   &initialMessages,
                    const unordered_set<u64> &actingCommanders)
{
    Action intendedAction = getRandomMessage();
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (actingCommanders.find(i) != actingCommanders.end()) continue;

        initialMessages[i][i] = intendedAction;
    }
}

void sendRandomOrders(const u64                &nGenerals,
                      vector<vector<Action>>   &initialMessages,
                      const unordered_set<u64> &actingCommanders)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (actingCommanders.find(i) != actingCommanders.end()) continue;

        initialMessages[i][i] = getRandomMessage();
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
                   vector<Role>                 &roles,
                   vector<vector<Action>>       &thisRoundMessages,
                   const vector<vector<Action>> &initialOrders)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        const Action senderMessage = initialOrders[i][i];
        if (actingCommanders.find(i) != actingCommanders.end()) continue;

        for (u64 j = 0; j < nGenerals; j++)
        {
            if (actingCommanders.find(j) != actingCommanders.end()) continue;

            if (thisRoundMessages[i][j] == Action::_NULL)
                thisRoundMessages[i][j] =
                    roles[i] == Role::FAITHFUL ? senderMessage : getRandomMessage();
        }
    }
}

void checkMajority(const u64 &commanderId, const u64 &nGenerals, const vector<vector<Action>> &messages, const vector<Role> &roles)
{
    vector<u64> totalAttackMessages  = vector<u64>(nGenerals, 0);
    vector<u64> totalRetreatMessages = vector<u64>(nGenerals, 0);
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (i == commanderId) continue;

        for (u64 j = 0; j < nGenerals; j++)
        {
            if (messages[i][j] == Action::ATTACK) totalAttackMessages[j] += 1;
            if (messages[i][j] == Action::RETREAT) totalRetreatMessages[j] += 1;
        }
    }

    vector<Action> decisions = vector<Action>(nGenerals, Action::_NULL);
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (totalAttackMessages > totalRetreatMessages)
            decisions[i] = Action::ATTACK;
        else
            decisions[i] = Action::RETREAT;
    }

    Action faithfulDecision = Action::_NULL;
    cout << "Decisions: ";
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (i == commanderId) continue;

        if (faithfulDecision == Action::_NULL && roles[i] == Role::FAITHFUL)
            faithfulDecision = decisions[i];

        if (faithfulDecision != Action::_NULL && roles[i] == Role::FAITHFUL && decisions[i] != faithfulDecision)
            assert(false);

        cout << (decisions[i] == Action::ATTACK ? "Attack, " : "Retreat, ");
    }
    cout << endl;

    cout << "Faithful Decision: " << (faithfulDecision == Action::ATTACK ? "Attack" : "Retreat") << endl;
}

Action executeRounds(u64                     nRounds,
                     u64                     nGenerals,
                     u64                     nTraitors,
                     vector<Role>           &roles,
                     unordered_set<u64>     &actingCommanders,
                     vector<vector<Action>> &initialOrders,
                     const u64              &actingCommander)
{
    if (nRounds == 0)
    {
        vector<vector<Action>> thisRoundMessages = vector<vector<Action>>(nGenerals, vector<Action>(nGenerals, Action::_NULL));
        relayMessages(nGenerals,
                      actingCommanders,
                      roles,
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
        sendInitialOrders(nGenerals, initialOrders, roles[actingCommander], actingCommanders);

        const Action agreedAction = executeRounds(nRounds - 1,
                                                  nGenerals,
                                                  nTraitors,
                                                  roles,
                                                  actingCommanders,
                                                  initialOrders,
                                                  actingCommander);

        thisRoundMessages[actingCommander][actingCommander] = agreedAction;

        actingCommanders.erase(actingCommander);
    }

    relayMessages(nGenerals,
                  actingCommanders,
                  roles,
                  thisRoundMessages,
                  thisRoundMessages);

    if (nRounds == nTraitors - 1)
    {
        cout << "roles: ";
        for (u64 i = 0; i < nGenerals; i++)
        {
            if (i == actingCommander) continue;

            cout << (roles[i] == Role::FAITHFUL ? "Faithful" : "Traitor") << ", ";
        }
        cout << endl;
        checkMajority(actingCommander, nGenerals, thisRoundMessages, roles);
    }

    return getMajority(thisRoundMessages, nGenerals);
}

int main()
{
    u64 nGenerals;
    u64 nTraitors;
    getInputs(nGenerals, nTraitors);

    for (u64 i = 0; i < 10000; i++)
    {
        cout << endl;
        cout << endl;
        cout << "=============================== start ===============================" << endl;
        u64                commanderID;
        unordered_set<u64> actingCommanders;
        vector<Role>       roles(nGenerals, Role::_NULL);

        assignRoles(nGenerals, nTraitors, commanderID, roles);
        debugSetup(nGenerals, commanderID, roles);

        const Role             commanderRole = roles[commanderID];
        vector<vector<Action>> initialOrders = vector<vector<Action>>(nGenerals, vector<Action>(nGenerals, Action::_NULL));

        actingCommanders.insert(commanderID);
        sendInitialOrders(nGenerals,
                          initialOrders,
                          commanderRole,
                          actingCommanders);

        executeRounds(nTraitors - 1, nGenerals, nTraitors, roles, actingCommanders, initialOrders, commanderID);

        cout << "================================ end ================================" << endl;
        cout << endl;
        cout << endl;
    }
}
