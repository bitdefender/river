import RiverTracer
import time
from typing import List, Dict, Set
import logging

class RiverStatsTextual:
    def __init__(self):
        # Updates on screen feedback
        self.SECONDS_BETWEEN_STATS = 10 # The interval in seconds to show stats from the running execution

        # The set of basic blocks found so far. This is used to evaluate how valuate are the inputs according to the heuristic
        # Be carefully on how you use it in RL or distributed environments
        self.AllBlocksFound: Set[int] = set()

    # Called by tracers to aggregate new stats in
    def onAddNewStatsFromTracer(self, blocksFound):
        self.AllBlocksFound.update(blocksFound)

    # Gets the partial results from a collection of tracers
    def UpdateOutputStats(self, startTime, currTime, collectorTracers : List[any], forceOutput = False):
        # TODO Bogdan: disable this output if user requested the graphical interface !
        newTime = time.time()
        secondsSinceLastUpdate = (newTime - currTime)
        secondsSinceStart = (newTime - startTime)
        if secondsSinceLastUpdate >= self.SECONDS_BETWEEN_STATS or forceOutput == True:

            for tracer in collectorTracers:
                tracer.throwStats(self)

            currTime = newTime
            logging.critical(f"After {secondsSinceStart:.2f}s Found {len(self.AllBlocksFound)} blocks '\n' at addresses {self.AllBlocksFound}")

        return currTime
