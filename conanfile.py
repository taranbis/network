from conan import ConanFile

class Yt7TCPHandler(ConanFile):
	generators = "CMakeToolchain", "CMakeDeps"
	settings = "os", "build_type", "arch", "compiler"

	def configure(self):
		if self.settings.compiler == "msvc":
			self.options["boost"].shared = False
			self.options["boost"].header_only = False

	def requirements(self):
		self.requires("boost/1.85.0")

