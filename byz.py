from typing import Tuple, List, Set, cast
import random
from enum import Enum


class Role(Enum):
    NONE = 0
    TRAITOR = -1
    LOYAL = 1


class Message(Enum):
    ATTACK = 1
    RETREAT = -1
    NONE = 0


def get_random_message() -> Message:
    return Message.ATTACK if random.random() < 0.5 else Message.RETREAT


def assign_roles(n_generals: int, n_traitors: int) -> Tuple[int, List[Role]]:
    roles = [Role.NONE for _ in range(n_generals)]
    for i in range(n_traitors):
        new_traitor_id = random.randint(0, n_generals - 1)
        while roles[new_traitor_id] == Role.TRAITOR:
            new_traitor_id = random.randint(0, n_generals - 1)

        roles[new_traitor_id] = Role.TRAITOR

    for i in range(n_generals):
        if roles[i] == Role.NONE:
            roles[i] = Role.LOYAL

    commander_id = random.randint(0, n_generals - 1)

    return (commander_id, roles)


def get_inputs() -> Tuple[int, int]:
    n_generals = int(input("Enter the number of generals: "))
    n_traitors = int(input("Enter the number of traitors: "))
    assert n_generals > 3 * n_traitors
    return (n_generals, n_traitors)


def init_messages(n_generals: int) -> List[List[Message]]:
    return [[Message.NONE for _ in range(n_generals)] for _ in range(n_generals)]


def exchange_messages(
    acting_commanders: Set[int],
    roles: List[Role],
    this_round_messages: List[List[Message]],
    messages_from_commander: List[Message],
) -> None:
    n_generals = len(roles)
    for sender in range(n_generals):
        if sender in acting_commanders:
            continue

        sender_message = messages_from_commander[sender]
        for receiver in range(n_generals):
            if receiver in acting_commanders:
                continue

            assert this_round_messages[sender][receiver] == Message.NONE

            this_round_messages[sender][receiver] = (
                sender_message if roles[sender] == Role.LOYAL else get_random_message()
            )


def get_majority_decisions(
    this_round_messages: List[List[Message]],
    n_generals: int,
    acting_commanders: Set[int],
) -> List[Message]:
    attack_messages = [0] * n_generals
    retreat_messages = [0] * n_generals

    for i in range(n_generals):
        if i in acting_commanders:
            continue

        for j in range(n_generals):
            if j in acting_commanders:
                continue

            if this_round_messages[i][j] == Message.ATTACK:
                attack_messages[j] += 1
            elif this_round_messages[i][j] == Message.RETREAT:
                retreat_messages[j] += 1

    majority_decisions = [Message.NONE for _ in range(n_generals)]
    for i in range(n_generals):
        if i in acting_commanders:
            continue

        if attack_messages[i] > retreat_messages[i]:
            majority_decisions[i] = Message.ATTACK
        elif (
            retreat_messages[i] > attack_messages[i]
            or retreat_messages[i] == attack_messages[i]
        ):
            majority_decisions[i] = Message.RETREAT

    return majority_decisions


def use_majority_decisions(
    new_commander,
    n_generals,
    acting_commanders,
    decisions,
    this_round_messages,
) -> None:
    for j in range(n_generals):
        if j in acting_commanders:
            continue

        this_round_messages[new_commander][j] = decisions[j]


def get_consensus(
    n_round: int,
    n_generals: int,
    roles: List[Role],
    acting_commanders: Set[int],
    messages_from_commander: List[Message],
) -> List[Message]:
    if n_round == 0:
        this_round_messages: List[List[Message]] = init_messages(n_generals)
        exchange_messages(
            acting_commanders, roles, this_round_messages, messages_from_commander
        )

        decisions: List[Message] = get_majority_decisions(
            this_round_messages,
            n_generals,
            acting_commanders,
        )

        return decisions

    this_round_messages: List[List[Message]] = init_messages(n_generals)
    for new_commander in range(n_generals):
        if new_commander in acting_commanders:
            continue

        message_received_from_prev_commander = messages_from_commander[new_commander]
        new_commander_messages: List[Message] = get_messages_based_on_role(
            n_generals,
            roles[new_commander],
            message_received_from_prev_commander,
            acting_commanders,
            new_commander,
        )

        acting_commanders.add(new_commander)

        decisions: List[Message] = get_consensus(
            n_round - 1,
            n_generals,
            roles,
            acting_commanders,
            new_commander_messages,
        )

        decisions[new_commander] = message_received_from_prev_commander

        acting_commanders.remove(new_commander)

        use_majority_decisions(
            new_commander,
            n_generals,
            acting_commanders,
            decisions,
            this_round_messages,
        )

    decisions: List[Message] = get_majority_decisions(
        this_round_messages,
        n_generals,
        acting_commanders,
    )

    return decisions


def get_decision_of_faithful_generals(
    n_generals: int,
    acting_commanders: Set[int],
    roles: List[Role],
    decisions: List[Message],
) -> Message:
    faithful_decision = Message.NONE

    for j in range(n_generals):
        if j in acting_commanders:
            continue

        if roles[j] == Role.LOYAL and faithful_decision == Message.NONE:
            faithful_decision = decisions[j]

        if roles[j] == Role.LOYAL:
            assert decisions[j] == faithful_decision

    assert faithful_decision != Message.NONE

    return faithful_decision


def get_messages_based_on_role(
    n_generals: int,
    curr_commander_role: Role,
    initial_message: Message,
    acting_commanders: Set[int],
    new_commander: int,
) -> List[Message]:
    messages = [Message.NONE for _ in range(n_generals)]

    for i in range(n_generals):
        if i in acting_commanders:
            continue

        messages[i] = (
            initial_message
            if (curr_commander_role == Role.LOYAL or i == new_commander)
            else get_random_message()
        )

    return messages


def byz() -> None:
    (n_generals, n_traitors) = cast(Tuple[int, int], get_inputs())

    iter = 10000
    for _ in range(iter):
        acting_commanders = set()

        (commander_id, roles) = cast(
            Tuple[int, List[Role]], assign_roles(n_generals, n_traitors)
        )

        initial_message: Message = get_random_message()

        commander_messages: List[Message] = get_messages_based_on_role(
            n_generals,
            roles[commander_id],
            initial_message,
            acting_commanders,
            commander_id,
        )

        acting_commanders.add(commander_id)

        decisions: List[Message] = get_consensus(
            n_traitors - 1,
            n_generals,
            roles,
            acting_commanders,
            commander_messages,
        )

        final_decision: Message = get_decision_of_faithful_generals(
            n_generals, acting_commanders, roles, decisions
        )

        if roles[commander_id] == Role.LOYAL:
            assert final_decision == initial_message

        acting_commanders.remove(commander_id)


if __name__ == "__main__":
    byz()
