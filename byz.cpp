#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace byz {
typedef uint64_t u64;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<u64> coinFlipDist(0, 1);

enum class Message {
  ATTACK,
  RETREAT,
  NONE,
};

enum class Role {
  LOYAL,
  TRAITOR,
  NONE,
};

inline bool flipCoin() {
  return coinFlipDist(gen);
}

inline Message getRandomMessage() {
  return flipCoin() ? Message::ATTACK : Message::RETREAT;
}

inline bool isActingCommander(const std::unordered_set<u64>& actingCommanders,
                              const u64& commanderID) {
  return actingCommanders.find(commanderID) != actingCommanders.end();
}

void assignRoles(const u64& nGenerals, const u64& nTraitors, u64& commanderID,
                 std::vector<Role>& roles) {
  std::uniform_int_distribution<u64> dis(0, nGenerals - 1);
  commanderID = dis(gen);

  for (u64 i = 0; i < nTraitors; i++) {
    u64 newTraitorID = dis(gen);
    while (roles[newTraitorID] == Role::TRAITOR) newTraitorID = dis(gen);

    roles[newTraitorID] = Role::TRAITOR;
  }

  for (u64 i = 0; i < nGenerals; i++) {
    if (roles[i] == Role::NONE) roles[i] = Role::LOYAL;
  }
}

std::tuple<u64, u64> getInputs() {
  u64 nGenerals, nTraitors;
  std::cout << "Enter number of generals: ";
  std::cin >> nGenerals;

  std::cout << "Enter number of traitors: ";
  std::cin >> nTraitors;

  assert(nGenerals != 0 && nTraitors != 0 && nGenerals > (3 * nTraitors));

  return std::make_tuple(nGenerals, nTraitors);
}

inline Message getDecisionOfFaithfulGenerals(
    const u64& nGenerals, const std::unordered_set<u64>& actingCommanders,
    const std::vector<Role>& roles, const std::vector<Message>& decisions) {
  Message faithfulDecision = Message::NONE;
  for (u64 j = 0; j < nGenerals; j++) {
    if (isActingCommander(actingCommanders, j)) continue;

    if (roles[j] == Role::LOYAL && faithfulDecision == Message::NONE)
      faithfulDecision = decisions[j];

    if (roles[j] == Role::LOYAL) assert(faithfulDecision == decisions[j]);
  }

  assert(faithfulDecision != Message::NONE);

  return faithfulDecision;
}

std::vector<Message> getMajorityDecisions(
    const std::vector<std::vector<Message>>& thisRoundMessageGrid,
    const u64& nGenerals, const std::unordered_set<u64>& actingCommanders) {
  std::vector<u64> attackMessages(nGenerals, 0);
  std::vector<u64> retreatMessages(nGenerals, 0);

  for (u64 i = 0; i < nGenerals; i++) {
    if (isActingCommander(actingCommanders, i)) continue;

    for (u64 j = 0; j < nGenerals; j++) {
      if (isActingCommander(actingCommanders, j)) continue;

      if (thisRoundMessageGrid[i][j] == Message::ATTACK)
        attackMessages[j]++;
      else if (thisRoundMessageGrid[i][j] == Message::RETREAT)
        retreatMessages[j]++;
    }
  }

  std::vector<Message> decisions(nGenerals, Message::NONE);
  for (u64 i = 0; i < nGenerals; i++) {
    if (attackMessages[i] > retreatMessages[i])
      decisions[i] = Message::ATTACK;
    else if (retreatMessages[i] > attackMessages[i] ||
             attackMessages[i] == retreatMessages[i])
      decisions[i] = Message::RETREAT;
  }

  return decisions;
}

void exchangeMessages(const std::unordered_set<u64>& actingCommanders,
                      const std::vector<Role>& roles,
                      std::vector<std::vector<Message>>& thisRoundMessages,
                      const std::vector<Message>& messagesFromCommander) {
  const u64 nGenerals = roles.size();
  for (u64 sender = 0; sender < nGenerals; sender++) {
    if (isActingCommander(actingCommanders, sender)) continue;

    Message senderMessage = messagesFromCommander[sender];
    for (u64 receiver = 0; receiver < nGenerals; receiver++) {
      if (isActingCommander(actingCommanders, receiver)) continue;

      assert(thisRoundMessages[sender][receiver] == Message::NONE);

      thisRoundMessages[sender][receiver] =
          roles[sender] == Role::LOYAL ? senderMessage : getRandomMessage();
    }
  }
}

inline std::vector<std::vector<Message>> initMessages(const u64& nGenerals) {
  return std::vector<std::vector<Message>>(
      nGenerals, std::vector<Message>(nGenerals, Message::NONE));
}

void useMajorityDecisions(const u64 commanderId, const u64 nGenerals,
                          const std::unordered_set<u64>& actingCommanders,
                          const std::vector<Message>& decisions,
                          std::vector<std::vector<Message>>& messages) {
  for (u64 i = 0; i < nGenerals; i++) {
    if (isActingCommander(actingCommanders, i)) continue;

    messages[commanderId][i] = decisions[i];
  }
}

std::vector<Message> getMessagesBasedOnRole(
    u64 nGenerals, Role currCommanderRole, Message initialMessage,
    const std::unordered_set<u64>& actingCommanders, u64 newCommander) {
  std::vector<Message> messages =
      std::vector<Message>(nGenerals, Message::NONE);

  for (u64 i = 0; i < nGenerals; i++) {
    if (isActingCommander(actingCommanders, i)) continue;

    messages[i] = currCommanderRole == Role::LOYAL || i == newCommander
                      ? initialMessage
                      : getRandomMessage();
  }

  return messages;
}

std::vector<Message> getConsensus(
    const u64 nRounds, const u64 nGenerals, const std::vector<Role>& roles,
    std::unordered_set<u64>& actingCommanders,
    const std::vector<Message>& messagesFromPrevCommander) {
  if (nRounds == 0) {
    std::vector<std::vector<Message>> thisRoundMessages =
        initMessages(nGenerals);
    exchangeMessages(actingCommanders, roles, thisRoundMessages,
                     messagesFromPrevCommander);

    auto decisions =
        getMajorityDecisions(thisRoundMessages, nGenerals, actingCommanders);

    return decisions;
  }

  std::vector<std::vector<Message>> thisRoundMessages = initMessages(nGenerals);
  for (u64 newCommanderId = 0; newCommanderId < nGenerals; newCommanderId++) {
    if (isActingCommander(actingCommanders, newCommanderId)) continue;

    Message messageReceivedFromPrevCommander =
        messagesFromPrevCommander[newCommanderId];
    std::vector<Message> newCommanderMessages = getMessagesBasedOnRole(
        nGenerals, roles[newCommanderId], messageReceivedFromPrevCommander,
        actingCommanders, newCommanderId);

    actingCommanders.insert(newCommanderId);

    std::vector<Message> decisions = getConsensus(
        nRounds - 1, nGenerals, roles, actingCommanders, newCommanderMessages);

    decisions[newCommanderId] = messageReceivedFromPrevCommander;

    actingCommanders.erase(newCommanderId);

    useMajorityDecisions(newCommanderId, nGenerals, actingCommanders, decisions,
                         thisRoundMessages);
  }

  std::vector<Message> decisions =
      getMajorityDecisions(thisRoundMessages, nGenerals, actingCommanders);

  return decisions;
}

void byz() {
  auto [nGenerals, nTraitors] = getInputs();

  const u64 iter = 10000;
  for (u64 i = 0; i < iter; i++) {
    u64 commanderId;
    std::unordered_set<u64> actingCommanders;
    std::vector<Role> roles(nGenerals, Role::NONE);
    assignRoles(nGenerals, nTraitors, commanderId, roles);

    const Message originalMessage = getRandomMessage();
    std::vector<Message> commanderMessages =
        getMessagesBasedOnRole(nGenerals, roles[commanderId], originalMessage,
                               actingCommanders, commanderId);

    actingCommanders.insert(commanderId);

    const std::vector<Message> decisions = getConsensus(
        nTraitors - 1, nGenerals, roles, actingCommanders, commanderMessages);

    Message finalDecision = getDecisionOfFaithfulGenerals(
        nGenerals, actingCommanders, roles, decisions);

    if (roles[commanderId] == Role::LOYAL)
      assert(finalDecision == originalMessage);

    actingCommanders.erase(commanderId);
  }
}

}  // namespace byz

int main() {
  byz::byz();
  return 0;
}
