import argparse
import random
from enum import Enum
from collections import Counter
from typing import List, Tuple

class Action(Enum):
    ATTACK = 'Attack'
    RETREAT = 'Retreat'
    NULL = None

class Role(Enum):
    FAITHFUL = 'Faithful'
    TRAITOR = 'Traitor'

def flip_coin():
    return random.choice([True, False])

def get_random_message():
    return Action.ATTACK if flip_coin() else Action.RETREAT

def assign_roles(n_generals: int, n_traitors: int) -> Tuple[int, List[Role]]:
    """Assign roles to generals and select a commander.

    Args:
        n_generals (int): Total number of generals.
        n_traitors (int): Number of traitors.

    Returns:
        Tuple[int, List[Role]]: Commander ID and list of roles.
    """
    roles = [Role.FAITHFUL] * n_generals
    traitors = random.sample(range(n_generals), n_traitors)
    for t in traitors:
        roles[t] = Role.TRAITOR
    commander_id = random.randint(0, n_generals - 1)
    return commander_id, roles

def send_initial_orders(commander_id: int, roles: List[Role], n_generals: int) -> List[Action]:
    """Send initial orders from the commander to all generals.

    If the commander is faithful, all generals receive the same order.
    If the commander is a traitor, each general receives a random order.

    Args:
        commander_id (int): ID of the commander.
        roles (List[Role]): List of roles for each general.
        n_generals (int): Total number of generals.

    Returns:
        List[Action]: Orders sent to each general.
    """
    orders = [Action.NULL] * n_generals
    commander_role = roles[commander_id]
    if commander_role == Role.FAITHFUL:
        action = get_random_message()
        orders = [action for _ in range(n_generals)]
    else:
        orders = [get_random_message() for _ in range(n_generals)]
    return orders

def relay_messages(orders: List[Action], roles: List[Role], n_generals: int) -> List[List[Action]]:
    """Relay messages among generals.

    Each general relays the order they received from the commander.
    If the general is faithful, they relay the order they received.
    If the general is a traitor, they send a random order.

    Args:
        orders (List[Action]): Orders sent to each general.
        roles (List[Role]): List of roles for each general.
        n_generals (int): Total number of generals.

    Returns:
        List[List[Action]]: Relayed messages from each general.
    """
    relayed = []
    for sender in range(n_generals):
        sender_msgs = []
        for receiver in range(n_generals):
            if roles[sender] == Role.FAITHFUL:
                sender_msgs.append(orders[sender])
            else:
                sender_msgs.append(get_random_message())
        relayed.append(sender_msgs)
    return relayed

def consensus(messages, n_generals):
    final_decisions = []
    for i in range(n_generals):
        counts = Counter(msg[i] for msg in messages)
        final_decisions.append(counts.most_common(1)[0][0])
    return Counter(final_decisions).most_common(1)[0][0]

def run_byzantine_simulation(n_generals, n_traitors, iterations=100):
    for _ in range(iterations):
        commander_id, roles = assign_roles(n_generals, n_traitors)
        initial_orders = send_initial_orders(commander_id, roles, n_generals)
        relayed_msgs = relay_messages(initial_orders, roles, n_generals)
        decision = consensus(relayed_msgs, n_generals)

        faithful_decisions = [decision for i, role in enumerate(roles) if role == Role.FAITHFUL]
        if len(set(faithful_decisions)) > 1:
            raise ValueError("Faithful generals failed to reach consensus.")

    print("Simulation completed successfully.")


# Example execution:
def main():
    parser = argparse.ArgumentParser(description="Byzantine Generals Simulation")
    parser.add_argument("-t", "--traitors", type=int, required=True, help="Number of traitors")
    parser.add_argument("-g", "--generals", type=int, required=False, help="Total number of generals. Default is 3 times the number of traitors plus 1.")
    args = parser.parse_args()

    
    run_byzantine_simulation(
        n_traitors = args.traitors,
        n_generals = args.generals if args.generals else 3 * args.traitors + 1,
        iterations=1000
    )

if __name__ == "__main__":
    main()
