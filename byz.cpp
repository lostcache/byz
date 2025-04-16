#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <tuple>
#include <unordered_set>
#include <vector>

typedef uint64_t u64;

namespace {
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<u64> coinFlipDist(0, 1);

enum class Action {
  ATTACK,
  RETREAT,
  NONE,
};

enum class Role {
  FAITHFUL,
  TRAITOR,
  NONE,
};

inline std::string toString(Action action) {
  switch (action) {
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

void toString(Role role) {
  switch (role) {
    case Role::FAITHFUL:
      std::cout << "FAITHFUL";
      break;
    case Role::TRAITOR:
      std::cout << "TRAITOR";
      break;
    case Role::NONE:
      std::cout << "NONE";
      break;
  }
}

inline void printDecision(const std::vector<Action>& decisions) {
  std::cout << "..............Decisions.................." << std::endl;
  for (auto it = decisions.begin(); it < decisions.end(); it++) {
    std::cout << (*it == Action::ATTACK    ? "A"
                  : *it == Action::RETREAT ? "R"
                                           : "X");
    std::cout << ", ";
  }
  std::cout << std::endl;
  std::cout << "..............Decisions.................." << std::endl;
}

inline void printSetup(const u64& nGenerals, const u64& commanderID,
                       std::vector<Role>& roles) {
  std::cout << "commander: " << commanderID << std::endl;

  std::cout << "roles: ";
  for (u64 i = 0; i < nGenerals; i++)
    std::cout << (roles[i] == Role::FAITHFUL  ? "Faithful"
                  : roles[i] == Role::TRAITOR ? "Traitor"
                                              : "?")
              << ", ";

  std::cout << std::endl;
  std::cout << std::endl;
}

inline void printMessages(const std::vector<std::vector<Action>>& messages) {
  for (u64 i = 0; i < messages.size(); i++) {
    for (u64 j = 0; j < messages[i].size(); j++) {
      std::cout << (messages[i][j] == Action::ATTACK    ? "A"
                    : messages[i][j] == Action::RETREAT ? "R"
                                                        : "?")
                << ", ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

inline void printActingCommanders(
    const std::unordered_set<u64>& actingCommanders) {
  std::cout << "Acting Commanders: ";
  for (auto it = actingCommanders.begin(); it != actingCommanders.end(); it++) {
    std::cout << *it << ", ";
  }
  std::cout << std::endl;
}

inline bool flipCoin() {
  return coinFlipDist(gen);
}

inline Action getRandomMessage() {
  return flipCoin() ? Action::ATTACK : Action::RETREAT;
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
    if (roles[i] == Role::NONE) roles[i] = Role::FAITHFUL;
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

inline Action getDecisionOfFaithfulGenerals(
    const u64& nGenerals, const std::unordered_set<u64>& actingCommanders,
    const std::vector<Role>& roles, const std::vector<Action>& decisions) {
  Action faithfulDecision = Action::NONE;
  for (u64 j = 0; j < nGenerals; j++) {
    if (isActingCommander(actingCommanders, j)) continue;

    if (roles[j] == Role::FAITHFUL && faithfulDecision == Action::NONE)
      faithfulDecision = decisions[j];

    if (roles[j] == Role::FAITHFUL) assert(faithfulDecision == decisions[j]);
  }

  assert(faithfulDecision != Action::NONE);

  return faithfulDecision;
}

std::vector<Action> getMajorityDecisions(
    const std::vector<std::vector<Action>>& thisRoundMessageGrid,
    const u64& nGenerals, const std::unordered_set<u64>& actingCommanders) {
  std::vector<u64> attackMessages(nGenerals, 0);
  std::vector<u64> retreatMessages(nGenerals, 0);

  for (u64 i = 0; i < nGenerals; i++) {
    if (isActingCommander(actingCommanders, i)) continue;

    for (u64 j = 0; j < nGenerals; j++) {
      if (isActingCommander(actingCommanders, j)) continue;

      if (thisRoundMessageGrid[i][j] == Action::ATTACK)
        attackMessages[j]++;
      else if (thisRoundMessageGrid[i][j] == Action::RETREAT)
        retreatMessages[j]++;
    }
  }

  std::vector<Action> decisions(nGenerals, Action::NONE);
  for (u64 i = 0; i < nGenerals; i++) {
    if (attackMessages[i] > retreatMessages[i])
      decisions[i] = Action::ATTACK;
    else if (retreatMessages[i] > attackMessages[i])
      decisions[i] = Action::RETREAT;
  }

  return decisions;
}

void relayConsensusToPreviousRound(
    const u64 nGenerals, const u64 actingCommander,
    std::unordered_set<u64> actingCommanders,
    const std::vector<Action> decisions,
    std::vector<std::vector<Action>>& prevRoundMessageGrid) {
  for (u64 i = 0; i < nGenerals; i++) {
    if (isActingCommander(actingCommanders, i)) continue;

    prevRoundMessageGrid[actingCommander][i] = decisions[i];
  }
}

void relayMessageToOtherGenerals(
    const std::unordered_set<u64>& actingCommanders,
    const std::vector<Role>& roles,
    std::vector<std::vector<Action>>& thisRoundMessages,
    const Action messageFromCommander) {
  const u64 nGenerals = roles.size();
  for (u64 i = 0; i < nGenerals; i++) {
    if (isActingCommander(actingCommanders, i)) continue;

    for (u64 j = 0; j < nGenerals; j++) {
      if (isActingCommander(actingCommanders, j)) continue;

      assert(thisRoundMessages[i][j] == Action::NONE);

      thisRoundMessages[i][j] = roles[i] == Role::FAITHFUL
                                    ? messageFromCommander
                                    : getRandomMessage();
    }
  }
}

inline std::vector<std::vector<Action>> initMessageGrid(const u64& nGenerals) {
  return std::vector<std::vector<Action>>(
      nGenerals, std::vector<Action>(nGenerals, Action::NONE));
}

void useDecision(const u64 commanderId, const u64 nGenerals,
                 const std::unordered_set<u64>& actingCommanders,
                 const std::vector<Action>& decisions,
                 std::vector<std::vector<Action>>& messages) {
  for (u64 i = 0; i < nGenerals; i++) {
    if (isActingCommander(actingCommanders, i)) continue;

    messages[commanderId][i] = decisions[i];
  }
}

std::vector<Action> getConsensus(const u64 nRounds, const u64 nGenerals,
                                 const std::vector<Role>& roles,
                                 std::unordered_set<u64>& actingCommanders,
                                 const Action messageFromPrevCommander) {
  // std::cout << "Round: " << nRounds << std::endl;
  // std::cout << "Prev Commander Message: " <<
  // toString(messageFromPrevCommander) << std::endl;
  // printActingCommanders(actingCommanders); std::cout << std::endl;

  if (nRounds == 0) {
    std::vector<std::vector<Action>> thisRoundMessageGrid =
        initMessageGrid(nGenerals);
    relayMessageToOtherGenerals(actingCommanders, roles, thisRoundMessageGrid,
                                messageFromPrevCommander);
    // std::cout << "Relayed Messages:" << std::endl;
    // debugMessages(thisRoundMessageGrid);
    auto decisions =
        getMajorityDecisions(thisRoundMessageGrid, nGenerals, actingCommanders);
    // std::cout << "Decisions:" << std::endl;
    // printDecision(decisions);
    return decisions;
  }

  std::vector<std::vector<Action>> thisRoundMessages =
      initMessageGrid(nGenerals);
  for (u64 newCommander = 0; newCommander < nGenerals; newCommander++) {
    if (isActingCommander(actingCommanders, newCommander)) continue;

    actingCommanders.insert(newCommander);

    const Action newMessage = roles[newCommander] == Role::FAITHFUL
                                  ? messageFromPrevCommander
                                  : getRandomMessage();
    std::vector<Action> decisions = getConsensus(nRounds - 1, nGenerals, roles,
                                                 actingCommanders, newMessage);
    // std::cout << "Decisions:" << std::endl;
    // printDecision(decisions);
    useDecision(newCommander, nGenerals, actingCommanders, decisions,
                thisRoundMessages);
    // std::cout << "this round messages after using decisions:" << std::endl;
    // debugMessages(thisRoundMessages);

    actingCommanders.erase(newCommander);
  }

  return getMajorityDecisions(thisRoundMessages, nGenerals, actingCommanders);
}

void byz() {
  auto [nGenerals, nTraitors] = getInputs();

  const u64 iter = 100000;
  for (u64 i = 0; i < iter; i++) {
    u64 commanderID;
    std::unordered_set<u64> actingCommanders;
    std::vector<Role> roles(nGenerals, Role::NONE);
    assignRoles(nGenerals, nTraitors, commanderID, roles);

    const Action originalOrder = getRandomMessage();

    // printSetup(nGenerals, commanderID, roles);

    actingCommanders.insert(commanderID);

    const std::vector<Action> decisions = getConsensus(
        nTraitors - 1, nGenerals, roles, actingCommanders, originalOrder);
    // std::cout << "original order: " << toString(originalOrder) << std::endl;
    Action finalDecision = getDecisionOfFaithfulGenerals(
        nGenerals, actingCommanders, roles, decisions);
    // std::cout << "final decision: " << toString(finalDecision) << std::endl;

    if (roles[commanderID] == Role::FAITHFUL)
      assert(finalDecision == originalOrder);

    actingCommanders.erase(commanderID);
  }
}

}  // namespace

int main() {
  byz();
  return 0;
}
