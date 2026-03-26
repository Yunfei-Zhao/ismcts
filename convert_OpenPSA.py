# convert the fault tree format from Open-PSA to a format that can be used by the IS-MCTS and MC algorithm

import re	# library that manages to find the stored value between quotations

# define the function
def convert_OpenPSA(inputmodelname):

    file = open(inputmodelname + '.xml')		# open the text file

    unique_be_name_list = []		# define an empty array thet will include the names for all the basic events
    unique_be_probs = []		# define an empty array thet will include the probabilities for all the basic events
    events_name_list = []   	# define an empty array thet will include the names for all the gates
    gates_list = []     # define an empty array thet will include the types for all the gates
    child_events_name_list = []     # define an empty array thet will include the children for all the gates
    te = None       # the name of the top event is currently None

    while True:		# start a while loop to keep reading the text file line by line
        content = file.readline()		# read a new line from the text file
        if "<define-gate name" in content:		# new gate is defined
            gate_name = re.findall('"([^"]*)"', content)[0]		# store gate's name in a dummy variable
            events_name_list.append(gate_name)	# add the new gate's name in an array
            children_per_gate_array = []	# define an empty array to store the children of the newly defined gate

            while content != "</define-gate>\n":	# while the newly defined gate hasn't been terminated (used to record the gate type and it's children)
                content = file.readline()	# read a new line from the text file
                if "<and>" in content:		# if the gate type is AND
                    gate_type = '*'		# store gate's type in a dummy variable
                    gates_list.append(gate_type)	# add the new gate's type in an array
                elif "<or>" in content:		# if the gate type is OR
                    gate_type = '+'		# store gate's type in a dummy variable
                    gates_list.append(gate_type)	# add the new gate's type in an array
                elif "<nand>" in content:		# if the gate type is NAND
                    gate_type = 'n*'		# store gate's type in a dummy variable
                    gates_list.append(gate_type)	# add the new gate's type in an array
                elif "<nor>" in content:		# if the gate type is NOR
                    gate_type = 'n+'		# store gate's type in a dummy variable
                    gates_list.append(gate_type)	# add the new gate's type in an array
                elif "<not>" in content:		# if the gate type is NOT
                    gate_type = 'n'		# store gate's type in a dummy variable
                    gates_list.append(gate_type)	# add the new gate's type in an array
                elif "<atleast min" in content:		# if the gate type is K-Out-Of-N
                    k_value = re.findall('"([^"]*)"', content)[0]		# store k in a dummy variable
                    gates_list.append(int(k_value))	# add the new gate's type in an array (and convert it from string to integer)

                if "<gate name" in content:		# if one of the children of the newly defined gate is another gate
                    child_name = re.findall('"([^"]*)"', content)[0]	# store the child's name in a dummy variable
                    children_per_gate_array.append(child_name)		# add the new child's name in an array that includes all of the children of the newly defined gate
                elif "<basic-event name" in content:	# if one of the children of the newly defined gate is a basic event
                    child_name = re.findall('"([^"]*)"', content)[0]	# store the child's name in a dummy variable
                    children_per_gate_array.append(child_name)		# add the new child's name in an array that includes all of the children of the newly defined gate

            child_events_name_list.append(children_per_gate_array)	# add the children arrays for each gate into one big array (an array of arrays)

        elif "<define-basic-event name" in content:		# new basic event is defined
            basic_event_name = re.findall('"([^"]*)"', content)[0]	# store basic event's name in a dummy variable
            unique_be_name_list.append(basic_event_name)		# add the basic event's name in an array
        elif "<float value" in content:		# probability of the newly defined basic event is defined
            basic_event_prob = re.findall('"([^"]*)"', content)[0]	# store basic event's probability in a dummy variable
            unique_be_probs.append(float(basic_event_prob))		# add the basic event's probability in an array (and convert it from string to float)

        elif not content:	# if there is no longer any text to be read
            break	# break out of the while loop

    file.close()	# close the text file

    # convert the fault tree to the ismcts format
    events_name_list_ordered = unique_be_name_list.copy()    # the names of the ordered events in the fault tree
    events_list_ordered = list(range(len(unique_be_name_list)))  # the indices of the ordered events in a fault tree
    child_events_list_ordered =[None] * len(unique_be_name_list)   # the child events corresponding to each parent event
    gates_list_ordered = [None] * len(unique_be_name_list) # the gate corresponding to each event
    done_with_ft_format = False # done with format conversion?
    while not done_with_ft_format:
        # for each event in the events_name_list
        for i, event in enumerate(events_name_list):
            # if the event's child events have all been added to the ordered events list
            if event not in events_name_list_ordered and all(child_event in events_name_list_ordered for child_event in child_events_name_list[i]):
                events_name_list_ordered.append(event)
                events_list_ordered.append(len(events_list_ordered))
                gates_list_ordered.append(gates_list[i])
                child_events_name = child_events_name_list[i]
                child_events = []
                for child_event_name in child_events_name:
                    child_events.append(events_name_list_ordered.index(child_event_name))
                child_events_list_ordered.append(child_events)
                # if the top event has been reached
                if len(events_list_ordered) == len(events_name_list) + len(unique_be_name_list):    # if the list that contains all of the nodes eventually has the same length as both lists that contain the gates & basic events
                    event = te  # then the last event is the top event
                    done_with_ft_format = True
                    break

    # write the unique basic events and the new representation fault tree to two files

    outputmodelname = 'converted_unique_be_name_list.txt'
    with open(outputmodelname, 'w') as output_file:
        file_len = len(unique_be_name_list)
        for i, name in enumerate(unique_be_name_list):
            output_file.write(name)
            if i != file_len - 1:
                output_file.write('\n')

    outputmodelname = 'converted_unique_be_probs.txt'
    with open(outputmodelname, 'w') as output_file:
        file_len = len(unique_be_probs)
        for i, prob in enumerate(unique_be_probs):
            output_file.write(str(prob))
            if i != file_len - 1:
                output_file.write('\n')

    outputmodelname = 'converted_events_name_list_ordered.txt'
    with open (outputmodelname, 'w') as output_file:
        file_len = len(events_name_list_ordered)
        for i, name in enumerate(events_name_list_ordered):
            output_file.write(name)
            if i != file_len - 1:
                output_file.write('\n')

    outputmodelname = 'converted_events_list_ordered.txt'
    with open (outputmodelname, 'w') as output_file:
        file_len = len(events_list_ordered)
        for i, event in enumerate(events_list_ordered):
            output_file.write(str(event))
            if i != file_len - 1:
                output_file.write('\n')

    outputmodelname = 'converted_child_events_list_ordered.txt'
    with open (outputmodelname, 'w') as output_file:
        file_len = len(child_events_list_ordered)
        for i, child_events in enumerate(child_events_list_ordered):
            if child_events is None:
                output_file.write('None')
            else:
                line_len = len(child_events)
                for j, child_event in enumerate(child_events):
                    output_file.write(str(child_event))
                    if j != line_len - 1:
                        output_file.write(' ')
            if i != file_len - 1:
                output_file.write('\n')

    outputmodelname = 'converted_gates_list_ordered.txt'
    with open (outputmodelname, 'w') as output_file:
        file_len = len(gates_list_ordered)
        for i, gate in enumerate(gates_list_ordered):
            if gate is None:
                output_file.write('b')
            elif isinstance(gate, int):
                output_file.write(str(gate))
            else:
                output_file.write(gate)
            if i != file_len - 1:
                output_file.write('\n')

    # Create a header file for the number of basic events and number of events
    # Define the values
    num_unique_bes = len(unique_be_probs)  # number of unique basic events
    num_events = len(events_list_ordered)      # number of events

    # Create the header file content
    header_content = f"""\
    #ifndef events_h
    #define events_h

    #define num_unique_bes {num_unique_bes}  // number of unique basic events in fault tree
    #define num_events {num_events}      // number of events in fault tree

    #endif // events_h
    """

    # Write the content to events.h
    with open("events.h", "w") as header_file:
        header_file.write(header_content)