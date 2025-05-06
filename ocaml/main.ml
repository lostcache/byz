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

  let random () = if Random.bool () then Attack else Retreat
end

module Byz = struct
  (* init messages to be sent to generals based on role *)
  let get_messages_based_on_role ~n_generals ~role ~commander_msg ~commander_id
      =
    match role with
    | Role.Traitor ->
        Array.init n_generals (fun i ->
            match i = commander_id with
            | true -> commander_msg
            | false -> Message.random () )
    | Role.Loyal -> Array.make n_generals commander_msg
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

  (* The Gossip Phase: every general relays the message that they received from
     current acting commander to other generals, based on theier role *)
  let exchange_messages ~acting_commanders ~roles ~messages_from_commander =
    (* relays the message received to other generals based on role *)
    let relay_message_to_other_generals ~n_generals ~role ~message =
      Array.init n_generals (fun receiver ->
          if IntHashtbl.mem acting_commanders receiver then Message.None
          else
            match role with
            | Role.Loyal -> message
            | Role.Traitor -> Message.random ()
            | Role.None -> failwith "critical" )
    in

    let n_generals = Array.length roles in
    Array.init n_generals (fun sender ->
        match IntHashtbl.mem acting_commanders sender with
        | true -> Array.make n_generals Message.None
        | false ->
            relay_message_to_other_generals ~n_generals ~role:roles.(sender)
              ~message:messages_from_commander.(sender) )

  (* init message grid *)
  let init_messages ~n_generals =
    Array.init n_generals (fun _ ->
        Array.init n_generals (fun _ -> Message.None) )

  (* returns majority out of the decisions that a generals receives from acting
     commander and what other genereals "say" that they received from the acting
     commander *)
  let get_majority_decisions ~message_grid ~n_generals ~acting_commanders =
    (* returns majority out of the decisions for a general *)
    let get_majority ~general_index =
      let attack_msg_count = ref 0 in
      let retreat_msg_count = ref 0 in

      for i = 0 to n_generals - 1 do
        match message_grid.(i).(general_index) with
        | Message.Attack -> incr attack_msg_count
        | Message.Retreat -> incr retreat_msg_count
        | Message.None -> ()
      done;

      if attack_msg_count > retreat_msg_count then Message.Attack
      else Message.Retreat
    in

    Array.init n_generals (fun general_index ->
        match IntHashtbl.mem acting_commanders general_index with
        | true -> Message.None
        | false -> get_majority ~general_index )

  (* updates the current message grid with the decisions received from the base
     case of the recursion (The Gossip Phase) *)
  let use_majority_decisions ~n_generals ~curr_commander ~acting_commanders
      ~decisions ~message_grid =
    for j = 0 to n_generals - 1 do
      match IntHashtbl.mem acting_commanders j with
      | true -> ()
      | false -> message_grid.(curr_commander).(j) <- decisions.(j)
    done;
    ()

  (* the main recursive concensus algorithm *)
  let rec get_consensus ~n_round ~n_generals ~roles ~acting_commanders
      ~messages_from_commander =
    match n_round with
    | 0 ->
        let this_round_messages =
          exchange_messages ~acting_commanders ~roles ~messages_from_commander
        in

        let decisions =
          get_majority_decisions ~message_grid:this_round_messages ~n_generals
            ~acting_commanders
        in

        decisions
    | _ ->
        let this_round_messages = init_messages ~n_generals in
        for new_commander = 0 to n_generals - 1 do
          match IntHashtbl.mem acting_commanders new_commander with
          | true -> ()
          | false ->
              let commander_msg = messages_from_commander.(new_commander) in

              let new_commander_messages =
                get_messages_based_on_role ~n_generals
                  ~role:roles.(new_commander) ~commander_msg
                  ~commander_id:new_commander
              in

              IntHashtbl.add acting_commanders new_commander "";

              let decisions =
                get_consensus ~n_round:(n_round - 1) ~n_generals ~roles
                  ~acting_commanders
                  ~messages_from_commander:new_commander_messages
              in

              decisions.(new_commander) <- commander_msg;

              IntHashtbl.remove acting_commanders new_commander;

              use_majority_decisions ~n_generals ~curr_commander:new_commander
                ~acting_commanders ~decisions ~message_grid:this_round_messages
        done;

        let decisions =
          get_majority_decisions ~message_grid:this_round_messages ~n_generals
            ~acting_commanders
        in

        decisions

  (* confirms that loyal generals agree on the same action and then returns
     it *)
  let get_decision_of_loyal_generals ~n_generals ~acting_commanders ~roles
      ~decisions =
    let decision_of_loyal_generals = ref Message.None in
    for j = 0 to n_generals - 1 do
      match IntHashtbl.mem acting_commanders j with
      | true -> ()
      | false -> (
          match roles.(j) = Role.Loyal with
          | true -> (
              match !decision_of_loyal_generals = Message.None with
              | true -> decision_of_loyal_generals := decisions.(j)
              | false -> assert (decisions.(j) = !decision_of_loyal_generals) )
          | false -> () )
    done;

    !decision_of_loyal_generals

  let run ~n_generals ~n_traitors =
    Random.self_init ();

    let commander_id = Random.int n_generals in

    let commander_msg = Message.random () in

    let roles = Array.make n_generals Role.Loyal in
    let roles = assign_traitors n_generals n_traitors roles in
    let initial_messages =
      get_messages_based_on_role ~n_generals ~role:roles.(commander_id)
        ~commander_msg ~commander_id
    in
    initial_messages.(commander_id) <- commander_msg;
    let acting_commanders = IntHashtbl.create n_generals in

    IntHashtbl.add acting_commanders commander_id "";

    let decisions =
      get_consensus ~n_round:(n_traitors - 1) ~n_generals ~roles
        ~acting_commanders ~messages_from_commander:initial_messages
    in
    let final_decision =
      get_decision_of_loyal_generals ~n_generals ~acting_commanders ~roles
        ~decisions
    in

    if roles.(commander_id) = Role.Loyal then
      assert (final_decision = commander_msg);

    IntHashtbl.remove acting_commanders commander_id;

    ()
end

let () =
  let get_inputs () =
    Printf.printf "Enter the number of generals: ";
    let n_generals = read_line () |> int_of_string in
    Printf.printf "Enter the number of traitors: ";
    let n_traitors = read_line () |> int_of_string in
    match n_generals < 3 * n_traitors with
    | true -> failwith "fatal: generals must be > 3 * number of traitors"
    | false -> (n_generals, n_traitors)
  in

  let n_generals, n_traitors = get_inputs () in
  for i = 0 to 10_000 do
    Byz.run ~n_generals ~n_traitors
  done;
  ()
