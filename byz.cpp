#include <iostream>
#include <cstdint>
#include <vector>
#include <cassert>
#include <random>
#include <algorithm>

using namespace std;

typedef uint64_t u64;
typedef double f64;

const bool attack   = true;
const bool faithful = true;

const bool retreat = false;
const bool traitor = false;

void debugSetup(const u64&     nGenerals,
                const u64&     nTraitors,
                const u64&     commanderID,
                vector<bool>&  roles)
{
    cout << "commander: " << commanderID << endl;

    cout << "roles: ";
    for (u64 i = 0; i < nGenerals; i++)
        cout << roles[i] << ", ";

    cout << endl;
}

void debugMessages(const u64&    nGenerals,
                   vector<u64>&  totalAttackMessages,
                   vector<u64>&  totalRetreatMessages)
{

    cout << "Initial Orders" << endl;
    cout << "attack messages: ";
    for (u64 i = 0; i < nGenerals; i++)
        cout << totalAttackMessages[i] << ", ";

    cout << endl;

    cout << "retreat messages: ";
    for (u64 i = 0; i < nGenerals; i++)
        cout << totalRetreatMessages[i] << ", ";

    cout << endl;;
    cout << "==============" << endl;
}

bool flipCoin()
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<u64> dis(0, 1);
    return dis(gen);
}

void assignRoles(const u64&     nGenerals,
                 const u64&     nTraitors,
                 u64&           commanderID,
                 vector<bool>&  roles)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<u64> dis(0, nGenerals - 1);
    commanderID = dis(gen);

    for (u64 i = 0; i < nTraitors; i++)
    {
        u64 newTraitorID = dis(gen);
        while (roles[newTraitorID] == traitor)
            newTraitorID = dis(gen);

        roles[newTraitorID] = traitor;
    }
}

void sendInitialOrders(const u64&     commanderID,
                       const u64&     nGenerals,
                       vector<u64>&   totalAttackMessages,
                       vector<u64>&   totalRetreatMessages,
                       vector<bool>&  roles)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (i == commanderID) continue;

        if (roles[commanderID] == traitor)
            flipCoin() ? totalAttackMessages[i]++ : totalRetreatMessages[i]++;
        else
            totalAttackMessages[i]++;
    }
}

void relayRandomMessages(const u64&           sender,
                         const u64&           receiver,
                         const vector<u64>&   prevRoundAttackMessages,
                         const vector<u64>&   prevRoundRetreatMessages,
                         vector<u64>&         thisRoundAttackMessages,
                         vector<u64>&         thisRoundRetreatMessages)
{
    u64 prevRoundMessageCount = prevRoundAttackMessages[sender] + prevRoundRetreatMessages[sender];
    for (u64 i = 0; i < prevRoundMessageCount; i++)
        flipCoin() ? thisRoundAttackMessages[receiver]++ : thisRoundRetreatMessages[receiver]++;

}

void relayMessages(const u64&           sender,
                   const u64&           receiver,
                   const vector<u64>&   prevRoundAttackMessages,
                   const vector<u64>&   prevRoundRetreatMessages,
                   vector<u64>&         thisRoundAttackMessages,
                   vector<u64>&         thisRoundRetreatMessages)
{
    thisRoundAttackMessages[receiver]  += prevRoundAttackMessages[sender];
    thisRoundRetreatMessages[receiver] += prevRoundRetreatMessages[sender];
}

void executeRound(const u64&           nGenerals,
                  const u64&           commanderID,
                  const vector<u64>&   prevRoundAttackMessages,
                  const vector<u64>&   prevRoundRetreatMessages,
                  vector<u64>&         thisRoundAttackMessages,
                  vector<u64>&         thisRoundRetreatMessages,
                  const vector<bool>&  roles)
{
    for (u64 sender = 0; sender < nGenerals; sender++)
    {
        if (sender == commanderID) continue;

        for (u64 receiver = 0; receiver < nGenerals; receiver++)
        {
            if (receiver == sender || receiver == commanderID) continue;

            if (roles[sender] == traitor)
                relayRandomMessages(sender,
                                    receiver,
                                    prevRoundAttackMessages,
                                    prevRoundRetreatMessages,
                                    thisRoundAttackMessages,
                                    thisRoundRetreatMessages);
            else
                relayMessages(sender,
                              receiver,
                              prevRoundAttackMessages,
                              prevRoundRetreatMessages,
                              thisRoundAttackMessages,
                              thisRoundRetreatMessages);
        }
    }
}

void consolidateMessages(const u64&           round,
                         const u64&           commanderID,
                         const u64&           nGenerals,
                         vector<u64>&         totalAttackMessages,
                         vector<u64>&         totalRetreatMessages,
                         const vector<u64>&   thisRoundAttackMessages,
                         const vector<u64>&   thisRoundRetreatMessages)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (i == commanderID) continue;
        assert(thisRoundAttackMessages[i] + thisRoundRetreatMessages[i] == (pow(nGenerals - 2, round)));
        totalAttackMessages[i]  += thisRoundAttackMessages[i];
        totalRetreatMessages[i] += thisRoundRetreatMessages[i];
    }
}

void executeRounds(const u64&           nGenerals,
                   const u64&           nTraitors,
                   const u64&           commanderID,
                   vector<u64>&         totalAttackMessages,
                   vector<u64>&         totalRetreatMessages,
                   const vector<bool>&  roles)
{
    vector<u64> prevRoundAttackMessages  = totalAttackMessages;
    vector<u64> prevRoundRetreatMessages = totalRetreatMessages;

    for (u64 round = 1; round < nTraitors + 1; round++)
    {
        vector<u64> thisRoundAttackMessages  = vector<u64>(nGenerals, 0);
        vector<u64> thisRoundRetreatMessages = vector<u64>(nGenerals, 0);

        executeRound(nGenerals,
                     commanderID,
                     prevRoundAttackMessages,
                     prevRoundRetreatMessages,
                     thisRoundAttackMessages,
                     thisRoundRetreatMessages,
                     roles);

        consolidateMessages(round,
                            commanderID,
                            nGenerals,
                            totalAttackMessages,
                            totalRetreatMessages,
                            thisRoundAttackMessages,
                            thisRoundRetreatMessages);

        prevRoundAttackMessages  = thisRoundAttackMessages;
        prevRoundRetreatMessages = thisRoundRetreatMessages;
    }
}

void getInputs(u64& nGenerals, u64& nTraitors) {
    cout << "Enter number of generals: ";
    cin >> nGenerals;

    cout << "Enter number of traitors: ";
    cin >> nTraitors;

    assert(nGenerals != 0 && nTraitors != 0 && nGenerals > (3 * nTraitors));
}

bool getFinalDecision(u64&               commanderID,
                      const vector<u64>& totalAttackMessages,
                      const vector<u64>& totalRetreatMessages)
{
    bool finalDecisionIsAttack = all_of(totalAttackMessages.begin(),
                                        totalAttackMessages.end(),
                                        [&](const u64& attackMessageCount)
                                        {
                                            size_t index = &attackMessageCount - &totalAttackMessages[0];
                                            if (index == commanderID) return true;
                                            return totalAttackMessages[index] > totalRetreatMessages[index];
                                        });

    bool finalDecisionIsRetreat = all_of(totalRetreatMessages.begin(),
                                         totalRetreatMessages.end(),
                                         [&](const u64& retreatMessageCount)
                                         {
                                             size_t index = &retreatMessageCount - &totalRetreatMessages[0];
                                             if (index == commanderID) return true;
                                             return totalRetreatMessages[index] > totalAttackMessages[index];
                                         });

    assert(finalDecisionIsRetreat || finalDecisionIsAttack);

    if (finalDecisionIsAttack)
        return attack;
    else
        return retreat;
}

void calculateMajorityMessageRatio(bool                finalDecision,
                                   const u64&          commanderID,
                                   const u64&          nGenerals,
                                   const vector<u64>&  totalAttackMessagesArr,
                                   const vector<u64>&  totalRetreatMessagesArr)
{
    for (u64 i = 0; i < nGenerals; i++)
    {
        if (i == commanderID)
        {
            cout << "-" << endl;
            continue;
        }

        f64 totalAttackMessages = totalAttackMessagesArr[i];
        f64 totalRetreatMessages = totalRetreatMessagesArr[i];
        f64 totalMessages = totalAttackMessages + totalRetreatMessages;

        finalDecision == attack ?
            cout << (totalAttackMessages / totalMessages) << ", " :
            cout << (totalRetreatMessages / totalMessages) << ", ";
    }

    cout << endl;
}

int main()
{
    u64 nGenerals;
    u64 nTraitors;
    getInputs(nGenerals, nTraitors);

    u64 commanderID;
    vector<bool> roles(nGenerals, faithful);
    assignRoles(nGenerals, nTraitors, commanderID, roles);
    debugSetup(nGenerals, nTraitors, commanderID, roles);

    vector<u64> totalAttackMessages   = vector<u64>(nGenerals, 0);
    vector<u64> totalRetreatMessages  = vector<u64>(nGenerals, 0);
    sendInitialOrders(commanderID,
                      nGenerals,
                      totalAttackMessages,
                      totalRetreatMessages,
                      roles);
    debugMessages(nGenerals, totalAttackMessages, totalRetreatMessages);

    executeRounds(nGenerals,
                  nTraitors,
                  commanderID,
                  totalAttackMessages,
                  totalRetreatMessages,
                  roles);
    debugMessages(nGenerals, totalAttackMessages, totalRetreatMessages);

    bool finalDecision = getFinalDecision(commanderID, totalAttackMessages, totalRetreatMessages);

    calculateMajorityMessageRatio(finalDecision,
                                  commanderID,
                                  nGenerals,
                                  totalAttackMessages,
                                  totalRetreatMessages);

    return 0;
}
