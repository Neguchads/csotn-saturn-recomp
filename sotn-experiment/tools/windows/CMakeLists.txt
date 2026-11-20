cmake_minimum_required(VERSION 3.20)
# O pacote SDL detecta a arquitetura usando C; habilitar C antes do projeto gerado.
project(sotn_windows LANGUAGES C CXX)
set(SOTN_RECOMP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../build/jp/recomp" CACHE PATH "Diretório do C++ gerado")
add_subdirectory("${SOTN_RECOMP_DIR}" game)
