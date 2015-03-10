#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

/**
   * @brief Read lines from file into a vector of strings.
   * @param in \p std::istream to read from.
   * @param it output iterator to append to.
   */
  template <typename OutputIterator>
  inline void read_lines_into_vector(std::istream& in, OutputIterator it)
  {
    for (std::string line; std::getline(in, line);)
      *it++ = line;
  }

  /**
   * @brief Read lines from file into a vector of strings.
   * @param filename name of file to read from
   * @param it output iterator to append to.
   */
  template <typename OutputIterator>
  inline void read_lines_into_vector(const std::string& filename, OutputIterator it)
  {
    std::ifstream ifs(filename);
    read_lines_into_vector(ifs, it);
    ifs.close();
  }

#endif
  