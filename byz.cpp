#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <tuple>
#include <unordered_set>
#include <vector>

using namespace std;

typedef uint64_t u64;

random_device                 rd;
mt19937                       gen(rd());
uniform_int_distribution<u64> coinFlipDist(0, 1);

enum class Action
{
    ATTACK,
    RETREAT,
    NONE,
};

enum class Role
{
    FAITHFUL,
    TRAITOR,
    NONE,
};

inline string toString(Action action)
{
    switch (action)
    {
        case Action::ATTACK:
            return "ATTACK";
            break;
        case Action::RETREAT:
            return "RETREAT";
            break;
        case Action::NONE:
            return "NONE";
            break;
    }
}

void toString(Role role)
{
    switch (role)
    {
        case Role::FAITHFUL:
            cout << "FAITHFUL";
            break;
        case Role::TRAITOR:
            cout << "TRAITOR";
            break;
        case Role::NONE:
            cout << "NONE";
            break;
    }
}

inline void printDecision(const vector<Action> &decisions)
{
    cout << "..............Decisions.................." << endl;
    for (auto it = decisions.begin(); it < decisions.end(); it++)
    {
        cout << (*it == Action::ATTACK ? "A" : *it == Action::RETREAT ? "R"
                                                                      : "X");
        cout << ", ";
    }
    cout << endl;
    cout << "..............Decisions.................." << endl;
}

inline void printSetup(const u64    &nGenerals,
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

inline void printMessages(const vector<vector<Action>> &messages)
{
    for (u64 i = 0; i < messages.size(); i++)
    {
        for (u64 j = 0; j < messages[i].size(); j++)
        {
            cout << (messages[i][j] == Action::ATTACK ? "A" : messages[i][j] == Action::RETREAT ? "R"
                                                                                                : "?")
                 << ", ";
        }
        cout << endl;
    }
    cout << endl;
}

inline void printActingCommanders(const unordered_set<u64> &actingCommanders)
{
    cout << "Acting Commanders: ";
    for (auto it = actingCommanders.begin(); it != actingCommanders.end(); it++)
    {
        cout << *it << ", ";
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

inline bool isActingCommander(const unordered_set<u64> &actingCommanders, const u64 &commanderID)
{
    return actingCommanders.find(commanderID) != actingCommanders.end();
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
        if (roles[i] == Role::NONE)
            roles[i] = Role::FAITHFUL;
    }
}

tuple<u64, u64> getInputs()
{
    u64 nGenerals, nTraitors;
    cout << "Enter number of generals: ";
    cin >> nGenerals;

    cout << "Enter number of traitors: ";
    cin >> nTraitors;

    assert(nGenerals != 0 && nTraitors != 0 && nGenerals > (3 * nTraitors));

    return make_tuple(nGenerals, nTraitors);
}

inline Action getDecisionOfFaithfulGenerals(const u64                &nGenerals,
                                            const unordered_set<u64> &actingCommanders,
                                            const vector<Role>       &roles,
                                            const vector<Action>     &decisions)
{
    Action faithfulDecision = Action::NONE;
    for (u64 j = 0; j < nGenerals; j++)
    {
        if (isActingCommander(actingCommanders, j)) continue;

        if (roles[j] == Role::FAITHFUL && faithfulDecision == Action::NONE)
            faithfulDecision = decisions[j];

        if (roles[j] == Role::FAITHFUL)
            assert(faithfulDecision == decisions[j]);
    }

    assert(faithfulDecision != Action::NONE);

    return faithfulDecision;
}

vector<Action> getMajorityDecisions(const vector<vector<Action>> &thisRoundMessageGrid,
                                    const u64                    &nGenerals,
                                    const unordered_set<u64>     &actingCommanders)
{
    vector<u64> attackMessages(nGenerals, 0);
    vector<u64> retreatMessages(nGenerals, 0);

    for (u64 i = 0; i < nGenerals; i++)
    {
        if (isActingCommander(actingCommanders, i)) continue;

        for (u64 j = 0; j < nGenerals; j++)
        {
            if (isActingCommander(actingCommanders, j)) continue;

            if (thisRoundMessageGrid[i][j] == Action::ATTACK)
                attackMessages[j]++;
            else if (thisRoundMessageGrid[i][j] == Action::RETREAT)
                retreatMessages[j]++;
        }
    }

    vector<Action> decisions(nGenerals, Action::NONE);
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (attackMessages[i] > retreatMessages[i])
            decisions[i] = Action::ATTACK;
        else if (retreatMessages[i] > attackMessages[i])
            decisions[i] = Action::RETREAT;
    }

    return decisions;
}

void relayConsensusToPreviousRound(const u64               nGenerals,
                                   const u64               actingCommander,
                                   unordered_set<u64>      actingCommanders,
                                   const vector<Action>    decisions,
                                   vector<vector<Action>> &prevRoundMessageGrid)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (isActingCommander(actingCommanders, i)) continue;

        prevRoundMessageGrid[actingCommander][i] = decisions[i];
    }
}

void relayMessageToOtherGenerals(const unordered_set<u64> &actingCommanders,
                                 const vector<Role>       &roles,
                                 vector<vector<Action>>   &thisRoundMessages,
                                 const Action              messageFromCommander)
{
    const u64 nGenerals = roles.size();
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (isActingCommander(actingCommanders, i)) continue;

        for (u64 j = 0; j < nGenerals; j++)
        {
            if (isActingCommander(actingCommanders, j)) continue;

            assert(thisRoundMessages[i][j] == Action::NONE);

            thisRoundMessages[i][j] = roles[i] == Role::FAITHFUL ? messageFromCommander : getRandomMessage();
        }
    }
}

inline vector<vector<Action>> initMessageGrid(const u64 &nGenerals)
{
    return vector<vector<Action>>(nGenerals, vector<Action>(nGenerals, Action::NONE));
}

void useDecision(const u64                &commanderId,
                 const u64                 nGenerals,
                 const unordered_set<u64> &actingCommanders,
                 const vector<Action>     &decisions,
                 vector<vector<Action>>   &messages)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (isActingCommander(actingCommanders, i)) continue;

        messages[commanderId][i] = decisions[i];
    }
}

vector<Action> getConsensus(const u64           nRounds,
                            const u64           nGenerals,
                            const vector<Role> &roles,
                            unordered_set<u64> &actingCommanders,
                            const Action        messageFromPrevCommander)
{
    // cout << "Round: " << nRounds << endl;
    // cout << "Prev Commander Message: " << toString(messageFromPrevCommander) << endl;
    // printActingCommanders(actingCommanders);
    // cout << endl;

    if (nRounds == 0)
    {
        vector<vector<Action>> thisRoundMessageGrid = initMessageGrid(nGenerals);
        relayMessageToOtherGenerals(actingCommanders, roles, thisRoundMessageGrid, messageFromPrevCommander);
        // cout << "Relayed Messages:" << endl;
        // debugMessages(thisRoundMessageGrid);
        auto decisions = getMajorityDecisions(thisRoundMessageGrid, nGenerals, actingCommanders);
        // cout << "Decisions:" << endl;
        // printDecision(decisions);
        return decisions;
    }

    vector<vector<Action>> thisRoundMessages = initMessageGrid(nGenerals);
    for (u64 newCommander = 0; newCommander < nGenerals; newCommander++)
    {
        if (isActingCommander(actingCommanders, newCommander)) continue;

        actingCommanders.insert(newCommander);

        const Action   newMessage = roles[newCommander] == Role::FAITHFUL ? messageFromPrevCommander : getRandomMessage();
        vector<Action> decisions  = getConsensus(nRounds - 1, nGenerals, roles, actingCommanders, newMessage);
        // cout << "Decisions:" << endl;
        // printDecision(decisions);
        useDecision(newCommander, nGenerals, actingCommanders, decisions, thisRoundMessages);
        // cout << "this round messages after using decisions:" << endl;
        // debugMessages(thisRoundMessages);

        actingCommanders.erase(newCommander);
    }

    return getMajorityDecisions(thisRoundMessages, nGenerals, actingCommanders);
}

void byz()
{
    auto [nGenerals, nTraitors] = getInputs();

    const u64 iter = 100000;
    for (u64 i = 0; i < iter; i++)
    {
        u64                commanderID;
        unordered_set<u64> actingCommanders;
        vector<Role>       roles(nGenerals, Role::NONE);
        assignRoles(nGenerals, nTraitors, commanderID, roles);

        const Action originalOrder = getRandomMessage();

        // printSetup(nGenerals, commanderID, roles);

        actingCommanders.insert(commanderID);

        const vector<Action> decisions = getConsensus(nTraitors - 1, nGenerals, roles, actingCommanders, originalOrder);
        // cout << "original order: " << toString(originalOrder) << endl;
        Action finalDecision = getDecisionOfFaithfulGenerals(nGenerals, actingCommanders, roles, decisions);
        // cout << "final decision: " << toString(finalDecision) << endl;

        if (roles[commanderID] == Role::FAITHFUL)
            assert(finalDecision == originalOrder);

        actingCommanders.erase(commanderID);
    }
}

int main()
{
    byz();
    return 0;
}
