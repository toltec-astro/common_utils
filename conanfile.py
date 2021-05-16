#!/usr/bin/env python

from conans import ConanFile, CMake


class CommonUtilsConan(ConanFile):
   settings = "build_type"
   requires = "fmt/[>=7.1]", "spdlog/[>=1.8]", "eigen/[>=3.3]", "ceres-solver/[>=2.0]"
   generators = "cmake"
   default_options = {"*:shared": True}

   def imports(self):
      self.copy("*.dll", dst="bin", src="bin") # From bin to bin
      self.copy("*.dylib*", dst="bin", src="lib") # From lib to bin

   def build(self):
      cmake = CMake(self)
      cmake.configure()
      cmake.build()
