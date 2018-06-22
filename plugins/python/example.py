import api

class PyController(api.BaseController):
	fps = 6
	def key_down(self, key, special):
		api.warn("Key pressed", key)
		return False
	
	def game_state(self):
		return "{python_test_running:1}"
	
	def __init__(self, *args, **kwargs):
		super().__init__(*args, **kwargs)
		self.cnt = 0
	
	def start_draw(self, *args):
		self.cnt += 1
		return api.DrawType.DEFAULT
	
	def start_frame(self, *args, **kwargs):
		self.cnt = 0

	def end_frame(self, *args, **kwargs):
		api.info('frame drawn with ', self.cnt, 'calls')
	
