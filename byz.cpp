/**
 * @file byz.cpp
 * @brief Implementation of the Byzantine Generals' Problem solution
 * 
 * This program simulates the Byzantine Generals' Problem, where a group of generals
 * must agree on a common battle plan (attack or retreat) despite some generals being
 * traitors who might relay false information. The implementation follows the algorithm
 * described by Lamport, Shostak, and Pease, where loyal generals follow a consensus
 * protocol to reach agreement.
 */

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

random_device rd;
mt19937 gen(rd());

/**
 * @brief Outputs debug information about the simulation setup
 * 
 * @param nGenerals Total number of generals in the simulation
 * @param nTraitors Number of traitor generals
 * @param commanderID ID of the commander general
 * @param roles Vector indicating whether each general is faithful or a traitor
 */
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

/**
 * @brief Outputs debug information about messages sent between generals
 * 
 * @param nGenerals Total number of generals in the simulation
 * @param totalAttackMessages Count of "attack" messages received by each general
 * @param totalRetreatMessages Count of "retreat" messages received by each general
 */
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

/**
 * @brief Generates a random boolean value (true/false)
 * 
 * @return Random boolean (true or false with equal probability)
 */
bool flipCoin()
{
    uniform_int_distribution<u64> dis(0, 1);
    return dis(gen);
}

/**
 * @brief Randomly assigns roles (faithful/traitor) to generals and selects a commander
 * 
 * @param nGenerals Total number of generals
 * @param nTraitors Number of traitors to assign
 * @param commanderID [out] Will contain the ID of the randomly selected commander
 * @param roles [out] Vector that will be populated with role assignments
 */
void assignRoles(const u64&     nGenerals,
                 const u64&     nTraitors,
                 u64&           commanderID,
                 vector<bool>&  roles)
{
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

/**
 * @brief Simulates the commander sending initial orders to all other generals
 * 
 * If the commander is faithful, they send "attack" to everyone.
 * If the commander is a traitor, they randomly send "attack" or "retreat" to each general.
 * 
 * @param commanderID ID of the commander
 * @param nGenerals Total number of generals
 * @param totalAttackMessages [out] Count of "attack" messages received by each general
 * @param totalRetreatMessages [out] Count of "retreat" messages received by each general
 * @param roles Vector indicating whether each general is faithful or a traitor
 */
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

/**
 * @brief Simulates a traitor general relaying random messages (attack/retreat)
 * 
 * @param sender ID of the sending general
 * @param receiver ID of the receiving general
 * @param prevRoundAttackMessages Count of "attack" messages from previous round
 * @param prevRoundRetreatMessages Count of "retreat" messages from previous round
 * @param thisRoundAttackMessages [out] "Attack" messages to be updated for current round
 * @param thisRoundRetreatMessages [out] "Retreat" messages to be updated for current round
 */
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

/**
 * @brief Simulates a faithful general relaying messages exactly as received
 * 
 * @param sender ID of the sending general
 * @param receiver ID of the receiving general
 * @param prevRoundAttackMessages Count of "attack" messages from previous round
 * @param prevRoundRetreatMessages Count of "retreat" messages from previous round
 * @param thisRoundAttackMessages [out] "Attack" messages to be updated for current round
 * @param thisRoundRetreatMessages [out] "Retreat" messages to be updated for current round
 */
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

/**
 * @brief Executes a single round of message exchanges between generals
 * 
 * Each general (except the commander) relays their received messages to all other generals
 * (except the commander and themselves). Faithful generals relay messages exactly as received,
 * while traitors relay random messages.
 * 
 * @param nGenerals Total number of generals
 * @param commanderID ID of the commander
 * @param prevRoundAttackMessages Count of "attack" messages from previous round
 * @param prevRoundRetreatMessages Count of "retreat" messages from previous round
 * @param thisRoundAttackMessages [out] "Attack" messages to be updated for current round
 * @param thisRoundRetreatMessages [out] "Retreat" messages to be updated for current round
 * @param roles Vector indicating whether each general is faithful or a traitor
 */
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

/**
 * @brief Adds the messages from the current round to the total message counts
 * 
 * @param round Current round number
 * @param commanderID ID of the commander
 * @param nGenerals Total number of generals
 * @param totalAttackMessages [out] Running total of "attack" messages to be updated
 * @param totalRetreatMessages [out] Running total of "retreat" messages to be updated
 * @param thisRoundAttackMessages "Attack" messages from the current round
 * @param thisRoundRetreatMessages "Retreat" messages from the current round
 */
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

/**
 * @brief Executes multiple rounds of message exchanges as required by the algorithm
 * 
 * The Byzantine Generals algorithm requires f+1 rounds where f is the number of traitors.
 * This function orchestrates all rounds of message exchanges.
 * 
 * @param nGenerals Total number of generals
 * @param nTraitors Number of traitor generals
 * @param commanderID ID of the commander
 * @param totalAttackMessages [out] Running total of "attack" messages
 * @param totalRetreatMessages [out] Running total of "retreat" messages
 * @param roles Vector indicating whether each general is faithful or a traitor
 */
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

/**
 * @brief Gets user input for the number of generals and traitors
 * 
 * Validates that the number of generals is more than three times the number of traitors,
 * which is a requirement for the Byzantine Generals algorithm to work correctly.
 * 
 * @param nGenerals [out] Will contain the total number of generals
 * @param nTraitors [out] Will contain the number of traitor generals
 */
void getInputs(u64& nGenerals, u64& nTraitors) {
    cout << "Enter number of generals: ";
    cin >> nGenerals;

    cout << "Enter number of traitors: ";
    cin >> nTraitors;

    assert(nGenerals != 0 && nTraitors != 0 && nGenerals > (3 * nTraitors));
}

/**
 * @brief Determines the final consensus decision (attack or retreat)
 * 
 * The decision is considered "attack" if all generals received more attack messages
 * than retreat messages. Otherwise, the decision is "retreat".
 * 
 * @param commanderID ID of the commander
 * @param totalAttackMessages Total count of "attack" messages received by each general
 * @param totalRetreatMessages Total count of "retreat" messages received by each general
 * @return true for attack, false for retreat
 */
bool getFinalDecision(u64&               commanderID,
                      const vector<u64>& totalAttackMessages,
                      const vector<u64>& totalRetreatMessages)
{
    bool finalDecisionIsAttack = all_of(totalAttackMessages.begin(),
                                        totalAttackMessages.end(),
                                        [&](const u64& attackMsg)
                                        {
                                            size_t index = &attackMsg - &totalAttackMessages[0];
                                            if (index == commanderID) return true;
                                            return totalAttackMessages[index] > totalRetreatMessages[index];
                                        });

    bool finalDecisionIsRetreat = all_of(totalRetreatMessages.begin(),
                                         totalRetreatMessages.end(),
                                         [&](const u64& retreatMsg)
                                         {
                                             size_t index = &retreatMsg - &totalRetreatMessages[0];
                                             if (index == commanderID) return true;
                                             return totalRetreatMessages[index] > totalAttackMessages[index];
                                         });

    assert(finalDecisionIsRetreat || finalDecisionIsAttack);

    if (finalDecisionIsAttack)
        return attack;
    else
        return retreat;
}

/**
 * @brief Calculates and displays the ratio of majority messages for each general
 * 
 * For each general, shows the ratio of messages that match the final decision.
 * This shows how strongly each general's received messages support the consensus.
 * 
 * @param finalDecision The consensus decision (attack or retreat)
 * @param commanderID ID of the commander
 * @param nGenerals Total number of generals
 * @param totalAttackMessagesArr Total "attack" messages received by each general
 * @param totalRetreatMessagesArr Total "retreat" messages received by each general
 */
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
            cout << "N/A, ";
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

/**
 * @brief Main function that orchestrates the Byzantine Generals simulation
 * 
 * Collects inputs, assigns roles, executes the message exchange rounds,
 * determines the final decision, and displays results.
 * 
 * @return 0 on successful completion
 */
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
