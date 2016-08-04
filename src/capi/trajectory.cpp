/* Chemfiles, an efficient IO library for chemistry file formats
* Copyright (C) 2015 Guillaume Fraux
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/
*/
// clang-format off

#include "chemfiles.h"
#include "chemfiles/Trajectory.hpp"
#include "chemfiles/capi.hpp"
using namespace chemfiles;

CHFL_TRAJECTORY* chfl_trajectory_open(const char* filename, char mode) {
    assert(filename != nullptr);
    CHFL_TRAJECTORY* trajectory = nullptr;
    CHFL_ERROR_GOTO(
        trajectory = new Trajectory(filename, mode);
    )
    return trajectory;
error:
    delete trajectory;
    return nullptr;
}

CHFL_TRAJECTORY* chfl_trajectory_with_format(const char* filename, char mode, const char* format) {
    assert(filename != nullptr);
    assert(format != nullptr);
    CHFL_TRAJECTORY* trajectory = nullptr;
    CHFL_ERROR_GOTO(
        trajectory = new Trajectory(filename, mode, format);
    )
    return trajectory;
error:
    delete trajectory;
    return nullptr;
}

int chfl_trajectory_read_step(CHFL_TRAJECTORY *file, size_t step, CHFL_FRAME* frame) {
    assert(file != nullptr);
    assert(frame != nullptr);
    CHFL_ERROR_CATCH(
        *frame = file->read_step(step);
    )
}

int chfl_trajectory_read(CHFL_TRAJECTORY *file, CHFL_FRAME *frame) {
    assert(file != nullptr);
    assert(frame != nullptr);
    CHFL_ERROR_CATCH(
        *frame = file->read();
    )
}

int chfl_trajectory_write(CHFL_TRAJECTORY *file, const CHFL_FRAME *frame) {
    assert(file != nullptr);
    assert(frame != nullptr);
    CHFL_ERROR_CATCH(
         file->write(*frame);
    )
}

int chfl_trajectory_set_topology(CHFL_TRAJECTORY *file, const CHFL_TOPOLOGY *topology) {
    assert(file != nullptr);
    assert(topology != nullptr);
    CHFL_ERROR_CATCH(
        file->set_topology(*topology);
    )
}

int chfl_trajectory_set_topology_file(CHFL_TRAJECTORY *file, const char* filename) {
    assert(file != nullptr);
    assert(filename != nullptr);
    CHFL_ERROR_CATCH(
        file->set_topology(std::string(filename));
    )
}

int chfl_trajectory_set_topology_with_format(CHFL_TRAJECTORY *file, const char* filename, const char* format) {
    assert(file != nullptr);
    assert(filename != nullptr);
    assert(format != nullptr);
    CHFL_ERROR_CATCH(
        file->set_topology(std::string(filename), std::string(format));
    )
}

int chfl_trajectory_set_cell(CHFL_TRAJECTORY *file, const CHFL_CELL *cell) {
    assert(file != nullptr);
    assert(cell != nullptr);
    CHFL_ERROR_CATCH(
        file->set_cell(*cell);
    )
}

int chfl_trajectory_nsteps(CHFL_TRAJECTORY *file, size_t *nsteps) {
    assert(file != nullptr);
    assert(nsteps != nullptr);
    CHFL_ERROR_CATCH(
        *nsteps = file->nsteps();
    )
}

int chfl_trajectory_sync(CHFL_TRAJECTORY *file) {
    assert(file != nullptr);
    CHFL_ERROR_CATCH(
        file->sync();
    )
}

int chfl_trajectory_close(CHFL_TRAJECTORY *file) {
    delete file;
    file = nullptr;
    return CHFL_SUCCESS;
}