module IntHashtbl = Hashtbl.Make (struct
  type t = int

  let equal = ( = )
  let hash = Hashtbl.hash
end)

module Role = struct
  type role = Traitor | Loyal | None

  let to_string role =
    match role with
    | Traitor -> "Traitor"
    | Loyal -> "Loyal"
    | None -> "None"
end

module Message = struct
  type message = Attack | Retreat | None

  let to_string message =
    match message with
    | Attack -> "Attack "
    | Retreat -> "Retreat"
    | None -> "None   "
end

module Byz = struct
  let debug_messages messages =
    Printf.printf "--------------------\n";
    Array.iter
      (fun msg_arr ->
        Array.iter
          (fun msg -> Printf.printf "%s, " (Message.to_string msg))
          msg_arr;
        Printf.printf "\n" )
      messages;
    ()

  let debug_act_cmd acting_commanders =
    Printf.printf "Acting Commanders: ";
    IntHashtbl.iter (fun key _ -> Printf.printf "%d, " key) acting_commanders;
    Printf.printf "\n";
    ()

  let get_random_message () =
    match Random.int 2 with
    | 0 -> Message.Attack
    | _ -> Message.Retreat

  let get_messages_based_on_role n_generals role commander_message commander_id
      =
    match role with
    | Role.Traitor ->
        Array.init n_generals (fun i ->
            if i = commander_id then commander_message
            else get_random_message () )
    | Role.Loyal -> Array.init n_generals (fun _ -> commander_message)
    | Role.None -> failwith "critical"

  let rec assign_traitors n_generals n_traitors roles =
    if n_traitors = 0 then roles
    else
      let traitor_index = Random.int n_generals in
      if roles.(traitor_index) = Role.Traitor then
        assign_traitors n_generals n_traitors roles
      else (
        roles.(traitor_index) <- Role.Traitor;
        assign_traitors n_generals (n_traitors - 1) roles )

  let exchange_messages acting_commanders roles messages_from_commander =
    let n_generals = Array.length roles in
    Array.init n_generals (fun sender ->
        if IntHashtbl.mem acting_commanders sender then
          (* do not partake in gossip *)
          Array.make n_generals Message.None
        else
          (* relay message received from commander to other generals *)
          let sender_message = messages_from_commander.(sender) in
          let sender_role = roles.(sender) in
          Array.init n_generals (fun receiver ->
              if IntHashtbl.mem acting_commanders receiver then Message.None
              else
                match sender_role with
                | Role.Loyal -> sender_message
                | Role.Traitor -> get_random_message ()
                | Role.None -> failwith "critical" ) )

  let init_messages n_generals =
    Array.init n_generals (fun _ ->
        Array.init n_generals (fun _ -> Message.None) )

  let get_majority_decisions this_round_messages n_generals acting_commanders =
    let attack_messages = Array.make n_generals 0 in
    let retreat_messages = Array.make n_generals 0 in

    for i = 0 to n_generals - 1 do
      if IntHashtbl.mem acting_commanders i then ()
      else
        for j = 0 to n_generals - 1 do
          if IntHashtbl.mem acting_commanders j then ()
          else
            match this_round_messages.(i).(j) with
            | Message.Attack -> attack_messages.(j) <- attack_messages.(j) + 1
            | Message.Retreat ->
                retreat_messages.(j) <- retreat_messages.(j) + 1
            | Message.None -> ()
        done
    done;

    let majority_decisions = Array.make n_generals Message.None in
    for i = 0 to n_generals - 1 do
      if IntHashtbl.mem acting_commanders i then ()
      else if attack_messages.(i) > retreat_messages.(i) then
        majority_decisions.(i) <- Message.Attack
      else majority_decisions.(i) <- Message.Retreat
    done;

    majority_decisions

  let use_majority_decisions new_commander n_generals acting_commanders
      decisions this_round_messages =
    for j = 0 to n_generals - 1 do
      if IntHashtbl.mem acting_commanders j then ()
      else this_round_messages.(new_commander).(j) <- decisions.(j)
    done;
    ()

  let rec get_consensus n_round n_generals roles acting_commanders
      messages_from_commander =
    debug_act_cmd acting_commanders;

    if n_round = 0 then (
      let this_round_messages =
        exchange_messages acting_commanders roles messages_from_commander
      in

      Printf.printf "Round: %d\n" n_round;
      Printf.printf "After exchanging messages: \n";
      debug_messages this_round_messages;

      let decisions =
        get_majority_decisions this_round_messages n_generals acting_commanders
      in

      Printf.printf "Decisions: ";
      Array.iter
        (fun msg -> Printf.printf "%s, " (Message.to_string msg))
        decisions;
      Printf.printf "\n";
      Printf.printf "--------------------\n";

      decisions )
    else
      let this_round_messages = init_messages n_generals in
      for new_commander = 0 to n_generals - 1 do
        if IntHashtbl.mem acting_commanders new_commander then ()
        else
          let message_received_from_prev_commander =
            messages_from_commander.(new_commander)
          in
          let new_commander_messages =
            get_messages_based_on_role n_generals roles.(new_commander)
              message_received_from_prev_commander new_commander
          in

          IntHashtbl.add acting_commanders new_commander "";

          let decisions =
            get_consensus (n_round - 1) n_generals roles acting_commanders
              new_commander_messages
          in

          decisions.(new_commander) <- message_received_from_prev_commander;

          IntHashtbl.remove acting_commanders new_commander;

          use_majority_decisions new_commander n_generals acting_commanders
            decisions this_round_messages;
          Printf.printf "After using decision: \n";
          debug_messages this_round_messages
      done;

      Printf.printf "Round: %d\n" n_round;
      Printf.printf "Finally: \n";
      debug_messages this_round_messages;

      let decisions =
        get_majority_decisions this_round_messages n_generals acting_commanders
      in

      Printf.printf "Decisions: ";
      Array.iter
        (fun msg -> Printf.printf "%s, " (Message.to_string msg))
        decisions;
      Printf.printf "\n";
      Printf.printf "--------------------\n";

      decisions

  let get_decision_of_faithful_generals n_generals acting_commanders roles
      decisions =
    Printf.printf "final decisions: ";
    Array.iter
      (fun msg -> Printf.printf "%s, " (Message.to_string msg))
      decisions;
    Printf.printf "\n";
    let faithful_decision = ref Message.None in
    for j = 0 to n_generals - 1 do
      if IntHashtbl.mem acting_commanders j then ()
      else if roles.(j) = Role.Loyal && !faithful_decision = Message.None then
        faithful_decision := decisions.(j)
      else if roles.(j) = Role.Loyal then assert (decisions.(j) <> Message.None)
      else ()
    done;

    !faithful_decision

  let byz i n_generals n_traitors =
    Random.self_init ();

    let commander_id = Random.int n_generals in
    Printf.printf "Commander index: %d\n" commander_id;

    let commander_message = get_random_message () in
    Printf.printf "Commander message: %s\n"
      (Message.to_string commander_message);

    let roles = Array.make n_generals Role.Loyal in
    let roles = assign_traitors n_generals n_traitors roles in
    Printf.printf "Roles: ";
    Array.iter (fun role -> Printf.printf "%s, " (Role.to_string role)) roles;
    Printf.printf "\n";

    Printf.printf "Commander role: %s \n" (Role.to_string roles.(commander_id));

    let initial_messages =
      get_messages_based_on_role n_generals roles.(commander_id)
        commander_message commander_id
    in
    initial_messages.(commander_id) <- commander_message;
    Printf.printf "Messages: ";
    Array.iter
      (fun message -> Printf.printf "%s, " (Message.to_string message))
      initial_messages;
    Printf.printf "\n";

    let acting_commanders = IntHashtbl.create n_generals in

    IntHashtbl.add acting_commanders commander_id "";

    let decisions =
      get_consensus (n_traitors - 1) n_generals roles acting_commanders
        initial_messages
    in
    let final_decision =
      get_decision_of_faithful_generals n_generals acting_commanders roles
        decisions
    in

    if roles.(commander_id) = Role.Loyal then (
      Printf.printf "Final Decision: %s \n" (Message.to_string final_decision);

      if final_decision <> commander_message then Printf.printf "%d\n" i;
      assert (final_decision = commander_message) );

    IntHashtbl.remove acting_commanders commander_id;

    ()
end

let () =
  let get_inputs () =
    Printf.printf "Enter the number of generals: ";
    let n_generals = read_line () |> int_of_string in
    Printf.printf "Enter the number of traitors: ";
    let n_traitors = read_line () |> int_of_string in
    if 3 * n_traitors > n_generals then
      failwith "Generals must be > 3 * number of traitors"
    else (n_generals, n_traitors)
  in

  let n_generals, n_traitors = get_inputs () in
  for i = 0 to 999 do
    Printf.printf "=================\n";
    Byz.byz i n_generals n_traitors;
    Printf.printf "=================\n"
  done;
  ()
