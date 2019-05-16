import traceback

class SlotBlocker(object):
   class BlockState(object):
       def __init__(self):
           self.block = False

   def __init__(self, block_state):
       self.block_state = block_state
       self.prev_block = self.block_state.block

   def __enter__(self):
       self.block_state.block = True
       return self

   def __exit__(self, exc_type, exc_value, tb):
       if exc_type is not None:
           traceback.print_exception(exc_type, exc_value, tb)
       self.block_state.block = self.prev_block
