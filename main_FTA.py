import os
import subprocess
from convert_OpenPSA import convert_OpenPSA

FT_name = 'RandSCRAM006'        # write the name of the fault tree
algorithm = 'IS-MCTS'           # select 'IS-MCTS' or 'MC'

convert_OpenPSA(FT_name)  # read and convert the fault tree into text files 

commands = [
    "gcc ismcts_alg.c -o ismcts_alg.exe",   # command 0 -> use GCC compiler to compile the IS-MCTS code
    "ismcts_alg.exe",                       # command 1 -> execute the compiled IS-MCTS program
    "gcc mc_alg.c -o mc_alg.exe",           # command 2 -> use GCC compiler to compile the MC code
    "mc_alg.exe",                           # command 3 -> execute the compiled MC program
]

if algorithm == 'IS-MCTS':
    subprocess.run(commands[0] , shell = True)  # execute command 0
    subprocess.run(commands[1] , shell = True)  # execute command 1
elif algorithm == 'MC':
    subprocess.run(commands[2] , shell = True)  # execute command 2
    subprocess.run(commands[3] , shell = True)  # execute command 3

results = []    # temp variable to save results from text file

with open('Results.txt', 'r') as file:
    # Iterate over each line in the results file
    for line in file:
        # Record the value of each line
        results.append(float(line))

# Print the results
print(f'Fault Tree Name: {FT_name}')
print(f'Algorithm: {algorithm}')
print(f'Number of Simulations = {int(results[0])}')
print(f'Number of True Top Event = {int(results[1])}')
print(f'Top Event Probability = {results[2]}')
print(f'Runtime = {results[3]} s')
print(f'Min Number of Simulations = {int(results[4])}')

# Delete the unnecessary generated files
os.remove('Results.txt')
os.remove('converted_child_events_list_ordered.txt')
os.remove('converted_events_list_ordered.txt')
os.remove('converted_events_name_list_ordered.txt')
os.remove('converted_gates_list_ordered.txt')
os.remove('converted_unique_be_name_list.txt')
os.remove('converted_unique_be_probs.txt')
os.remove('events.h')
if algorithm == 'IS-MCTS':
    os.remove('ismcts_alg.exe')
elif algorithm == 'MC':
    os.remove('mc_alg.exe')