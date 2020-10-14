#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include "glslang/Public/ShaderLang.h"

#include <ghc/filesystem.hpp>
using namespace std;

class GLSLIncluder : public glslang::TShader::Includer {
  public:
    GLSLIncluder() : externalLocalDirectoryCount(0) { }
      virtual IncludeResult* includeLocal(const char* header, const char* includer, size_t depth) override {
        return readPath(header, includer, depth);
      }

      virtual IncludeResult* includeSystem(const char* header, const char* includer, size_t depth) override {
        return readPath(header, includer, depth);
      }

      virtual void pushExternalLocalDirectory(const std::string& dir) {
        directoryStack.push_back(dir);
        externalLocalDirectoryCount = (int)directoryStack.size();
      }

      virtual void releaseInclude(IncludeResult* result) override {
        if (result != nullptr) {
          delete [] static_cast<char*>(result->userData);
          delete result;
        }
      }

      virtual ~GLSLIncluder() override { }

      std::vector<std::string> dependencies;
  protected:
    std::vector<std::string> directoryStack;

    size_t externalLocalDirectoryCount;

    // Search for a valid "local" path based on combining the stack of include
    // directories and the nominal name of the header.
    virtual IncludeResult* readPath(const char* header, const char* includer, size_t depth) {

      // Discard popped include directories, and initialize when at parse-time first level.
      directoryStack.resize(depth + externalLocalDirectoryCount);
      if (depth == 1) {
        directoryStack.back() = ghc::filesystem::path(includer).remove_filename();
      }

      // Find a directory that works, using a reverse search of the include stack.
      for (auto it = directoryStack.rbegin(); it != directoryStack.rend(); ++it) {
        ghc::filesystem::path p(*it);
        p = ghc::filesystem::absolute(p / header);

        std::ifstream file(p, std::ios_base::binary | std::ios_base::ate);
        if (file) {
          this->dependencies.push_back(p);
          directoryStack.push_back(p.remove_filename());
          return newIncludeResult(p, file, (int)file.tellg());
        }
      }

      return nullptr;
    }

    // Do actual reading of the file, filling in a new include result.
    virtual IncludeResult* newIncludeResult(const std::string& path, std::ifstream& file, size_t length) const {
        char* content = new char[length];
        file.seekg(0, file.beg);
        file.read(content, length);
        return new IncludeResult(path, content, length, content);
    }
};