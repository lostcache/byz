from typing import Tuple, List, Set
import random
from enum import Enum


class Role(Enum):
    NONE = 0
    TRAITOR = -1
    FAITHFUL = 1


class Action(Enum):
    ATTACK = (1,)
    RETREAT = (-1,)
    NONE = (0,)


def get_random_message() -> Action:
    """
    Generate a random attack or retreat message with equal probability.
    
    Returns:
        Action: Randomly chosen action (ATTACK or RETREAT)
    """
    return Action.ATTACK if random.random() < 0.5 else Action.RETREAT


def assign_roles(n_generals: int, n_traitors: int) -> Tuple[int, List[Role]]:
    """
    Randomly assign roles to generals and select a commander.
    
    Args:
        n_generals: Total number of generals
        n_traitors: Number of traitor generals
        
    Returns:
        Tuple[int, List[Role]]: Commander ID and list of roles for all generals
    """
    roles = [Role.NONE for _ in range(n_generals)]
    for i in range(n_traitors):
        new_traitor_id = random.randint(0, n_generals - 1)
        while roles[new_traitor_id] == Role.TRAITOR:
            new_traitor_id = random.randint(0, n_generals - 1)

        roles[new_traitor_id] = Role.TRAITOR

    for i in range(n_generals):
        if roles[i] == Role.NONE:
            roles[i] = Role.FAITHFUL

    commander_id = random.randint(0, n_generals - 1)

    return (commander_id, roles)


def get_inputs() -> Tuple[int, int]:
    """
    Get user input for the number of generals and traitors.
    Ensures the Byzantine agreement condition (n_generals > 3*n_traitors) is met.
    
    Returns:
        Tuple[int, int]: Number of generals and number of traitors
    """
    n_generals = int(input("Enter the number of generals: "))
    n_traitors = int(input("Enter the number of traitors: "))
    assert n_generals > 3 * n_traitors
    return (n_generals, n_traitors)


def init_messages(n_generals: int) -> List[List[Action]]:
    """
    Initialize a 2D message matrix with NONE actions.
    
    Args:
        n_generals: Number of generals
        
    Returns:
        List[List[Action]]: n_generals x n_generals matrix filled with Action.NONE
    """
    return [[Action.NONE for _ in range(n_generals)] for _ in range(n_generals)]


def relay_messages_to_other_generals(
    acting_commanders: Set[int],
    roles: List[Role],
    this_round_messages: List[List[Action]],
    message_from_commander: Action,
) -> None:
    """
    Relay messages from commanders to other generals based on their roles.
    Faithful generals relay the original message, while traitors send random messages.
    
    Args:
        acting_commanders: Set of commander IDs who have already acted
        roles: List of roles for all generals
        this_round_messages: 2D matrix of messages being passed between generals
        message_from_commander: Original message from the commander
    """
    n_generals = len(roles)
    for i in range(n_generals):
        if i in acting_commanders:
            continue

        for j in range(n_generals):
            if j in acting_commanders:
                continue

            assert this_round_messages[i][j] is Action.NONE

            this_round_messages[i][j] = (
                message_from_commander
                if roles[i] == Role.FAITHFUL
                else get_random_message()
            )


def get_majority_decisions(
    this_round_messages: List[List[Action]],
    n_generals: int,
    acting_commanders: Set[int],
) -> List[Action]:
    """
    Calculate majority decisions for each general based on received messages.
    
    Args:
        this_round_messages: 2D matrix of messages exchanged between generals
        n_generals: Number of generals
        acting_commanders: Set of commander IDs who have already acted
        
    Returns:
        List[Action]: Final decisions for each general based on majority vote
    """
    attack_messages = [0] * n_generals
    retreat_messages = [0] * n_generals

    for i in range(n_generals):
        for j in range(n_generals):
            if this_round_messages[i][j] == Action.ATTACK:
                attack_messages[i] += 1
            elif this_round_messages[i][j] == Action.RETREAT:
                retreat_messages[i] += 1

    return [
        Action.ATTACK if attack_messages[i] > retreat_messages[i] else Action.RETREAT
        for i in range(n_generals)
    ]


def use_majority_decisions(
    new_commander, n_generals, acting_commanders, decisions, this_round_messages
) -> None:
    """
    Update the message matrix with majority decisions for a new commander.
    
    Args:
        new_commander: ID of the new commander
        n_generals: Number of generals
        acting_commanders: Set of commander IDs who have already acted
        decisions: List of decisions made by each general
        this_round_messages: 2D matrix of messages to update
    """
    for i in range(n_generals):
        if i in acting_commanders:
            continue

        this_round_messages[new_commander][i] = decisions[i]


def get_consensus(
    n_round, n_generals, roles, acting_commanders, message_from_prev_commander
) -> List[Action]:
    """
    Recursively compute consensus among generals using the Byzantine algorithm.
    
    Args:
        n_round: Current round number (decreases with each recursive call)
        n_generals: Number of generals
        roles: List of roles for all generals
        acting_commanders: Set of commander IDs who have already acted
        message_from_prev_commander: Message from the previous commander
        
    Returns:
        List[Action]: Final decisions for each general after consensus
    """
    if n_round == 0:
        this_round_messages: List[List[Action]] = init_messages(n_generals)
        relay_messages_to_other_generals(
            acting_commanders, roles, this_round_messages, message_from_prev_commander
        )

        return get_majority_decisions(
            this_round_messages,
            n_generals,
            acting_commanders,
        )

    this_round_messages: List[List[Action]] = init_messages(n_generals)
    for new_commander in range(n_generals):
        if new_commander in acting_commanders:
            continue

        acting_commanders.add(new_commander)

        new_commander_message = (
            message_from_prev_commander
            if roles[new_commander] == Role.FAITHFUL
            else get_random_message()
        )

        decisions = get_consensus(
            n_round - 1,
            n_generals,
            roles,
            acting_commanders,
            new_commander_message,
        )

        use_majority_decisions(
            new_commander, n_generals, acting_commanders, decisions, this_round_messages
        )

        acting_commanders.remove(new_commander)

    return get_majority_decisions(
        this_round_messages,
        n_generals,
        acting_commanders,
    )


def get_decision_of_faithful_generals(
    n_generals: int,
    acting_commanders: Set[int],
    roles: List[Role],
    decisions: List[Action],
) -> Action:
    """
    Determine the consensus decision among faithful generals.
    Verifies that all faithful generals have reached the same decision.
    
    Args:
        n_generals: Number of generals
        acting_commanders: Set of commander IDs who have already acted
        roles: List of roles for all generals
        decisions: List of decisions made by each general
        
    Returns:
        Action: The consensus decision among faithful generals
    """
    faithful_decision = Action.NONE

    for i in range(n_generals):
        if i in acting_commanders:
            continue

        if roles[i] == Role.FAITHFUL and faithful_decision == Action.NONE:
            faithful_decision = decisions[i]

        if roles[i] == Role.FAITHFUL:
            assert decisions[i] is faithful_decision

    assert faithful_decision is not Action.NONE

    return faithful_decision


def byz() -> None:
    """
    Main function implementing the Byzantine Generals Problem algorithm.
    
    Gets user inputs, assigns roles, and runs the consensus algorithm multiple times
    to verify that faithful generals always reach consensus when n_generals > 3*n_traitors.
    """
    (n_generals, n_traitors) = get_inputs()

    iter = 10000
    for _ in range(iter):
        acting_commanders = set()
        (commander_id, roles) = assign_roles(n_generals, n_traitors)

        original_order = get_random_message()

        acting_commanders.add(commander_id)

        decisions: List[Action] = get_consensus(
            n_traitors - 1,
            n_generals,
            roles,
            acting_commanders,
            original_order,
        )

        final_decision: Action = get_decision_of_faithful_generals(
            n_generals, acting_commanders, roles, decisions
        )

        if roles[commander_id] == Role.FAITHFUL:
            assert final_decision is original_order

        acting_commanders.remove(commander_id)


if __name__ == "__main__":
    byz()
