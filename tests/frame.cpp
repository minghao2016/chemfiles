#include <catch.hpp>
#include "chemfiles.hpp"
#include "chemfiles/Frame.hpp"
#include "chemfiles/UnitCell.hpp"
using namespace chemfiles;

TEST_CASE("Frame class usage", "[Frame]"){
    auto frame = Frame(10);

    SECTION("Contructor"){
        CHECK(frame.positions().capacity() == 10);
        CHECK(frame.cell().type() == UnitCell::INFINITE);
    }

    SECTION("Get and set"){
        CHECK(frame.step() == 0);
        frame.set_step(1000);
        CHECK(frame.step() == 1000);

        frame.set_cell(UnitCell(10));
        CHECK(frame.cell().type() == UnitCell::ORTHOROMBIC);

        // We already have dummy atoms in the topology
        CHECK(frame.topology().natom_types() == 1);
        frame.topology().append(Atom("H"));
        CHECK(frame.topology().natom_types() == 2);

        frame.resize(15);
        CHECK(frame.natoms() == 15);
        // No velocity data yet
        CHECK_FALSE(frame.velocities());

        frame.velocities() = Array3D(15);
        CHECK(frame.velocities());

        frame.positions()[0] = vector3d(1, 2, 3);
        CHECK(frame.positions()[0] == vector3d(1, 2, 3));

        (*frame.velocities())[0] = vector3d(5, 6, 7);
        CHECK((*frame.velocities())[0] == vector3d(5, 6, 7));

        Array3D data(10);
        for (size_t i=0; i<10; i++) {
            data[i] = vector3d(4.f, 3.4f, 1.f);
        }
        frame.set_positions(data);
        frame.set_velocities(data);

        auto positions = frame.positions();
        auto velocities = frame.velocities();

        for (size_t i=0; i<10; i++){
            CHECK(positions[i] == vector3d(4.f, 3.4f, 1.f));
            CHECK((*velocities)[i] == vector3d(4.f, 3.4f, 1.f));
        }
    }

    SECTION("Guess bonds"){
        Trajectory file(SRCDIR "/data/xyz/methane.xyz");

        file >> frame;
        frame.guess_topology();

        auto topology =  frame.topology();
        for (size_t i=1; i<5; i++){
            CHECK(topology.isbond(0, i));
        }

        CHECK_FALSE(topology.isbond(2, 4));
        CHECK_FALSE(topology.isbond(1, 2));

        CHECK(topology.isangle(1, 0, 2));
        CHECK(topology.isangle(3, 0, 2));
        CHECK(topology.isangle(2, 0, 4));

        CHECK(topology.bonds().size() == 4);
    }
}
