import json

def read_file(filename):

    with open(filename, 'r') as f:
        lines = f.read()

    sims = lines.split('\n\n')
    sims = sims[:-1]
    
    results = {
        'Okay': 0,
        'app_error': 0,
        'overflow': 0,
        'lock': 0
    }

    for sim in sims:
        sim_no, sim_det = sim.split('\n')

        problems = sim_det.split(', ')
        for problem in problems:
            results[problem] += 1

    return results

for i in range(1, 10):
    results = read_file('summary' + str(i))
    print("Summary", i)
    print(json.dumps(results, indent=4), end='\n\n')
    
