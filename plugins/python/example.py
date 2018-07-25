import api

class PyController(api.BaseController):
	fps = 6
	
	def __init__(self, *args, **kwargs):
		super().__init__(*args, **kwargs)
		self.cnt = 0

	def on_key_down(self, key, special):
		api.warn("Key pressed", key)
		return False
	
	def provide_game_state(self):
		return "{python_test_running:1}"
	
	def on_begin_draw(self, *args):
		self.cnt += 1
	
	def on_begin_frame(self, *args, **kwargs):
		self.cnt = 0

	def on_end_frame(self, *args, **kwargs):
		api.info('frame drawn with ', self.cnt, 'calls')
	
