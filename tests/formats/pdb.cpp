// Chemfiles, a modern library for chemistry file reading and writing
// Copyright (C) Guillaume Fraux and contributors -- BSD license

#include <fstream>
#include <cstdlib>
#include "catch.hpp"
#include "helpers.hpp"
#include "chemfiles.hpp"
using namespace chemfiles;

template <typename T>
static bool contains(const std::vector<T> haystack, const T& needle) {
    return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

TEST_CASE("Read files in PDB format") {
    SECTION("Read next step") {
        auto file = Trajectory("data/pdb/water.pdb");
        REQUIRE(file.nsteps() == 100);

        auto frame = file.read();

        REQUIRE(frame.size() == 297);
        auto positions = frame.positions();
        CHECK(approx_eq(positions[0], Vector3D(0.417, 8.303, 11.737), 1e-3));
        CHECK(approx_eq(positions[296], Vector3D(6.664, 11.6148, 12.961), 1e-3));

        auto cell = frame.cell();
        CHECK(cell.shape() == UnitCell::ORTHORHOMBIC);
        CHECK(approx_eq(cell.a(), 15.0, 1e-5));

        file.read(); // Skip a frame
        frame = file.read();

        REQUIRE(frame.size() == 297);
        positions = frame.positions();
        CHECK(approx_eq(positions[0], Vector3D(0.299, 8.310, 11.721), 1e-4));
        CHECK(approx_eq(positions[296], Vector3D(6.798, 11.509, 12.704), 1e-4));
    }

    SECTION("Read a specific step") {
        auto file = Trajectory("data/pdb/water.pdb");

        auto frame = file.read_step(2);
        REQUIRE(frame.size() == 297);
        auto positions = frame.positions();
        CHECK(approx_eq(positions[0], Vector3D(0.299, 8.310, 11.721), 1e-4));
        CHECK(approx_eq(positions[296], Vector3D(6.798, 11.509, 12.704), 1e-4));

        frame = file.read_step(0);
        REQUIRE(frame.size() == 297);
        positions = frame.positions();
        CHECK(approx_eq(positions[0], Vector3D(0.417, 8.303, 11.737), 1e-3));
        CHECK(approx_eq(positions[296], Vector3D(6.664, 11.6148, 12.961), 1e-3));

        auto cell = frame.cell();
        CHECK(cell.shape() == UnitCell::ORTHORHOMBIC);
        CHECK(approx_eq(cell.a(), 15.0, 1e-5));
    }

    SECTION("Read bonds") {
        auto file = Trajectory("data/pdb/MOF-5.pdb");
        auto frame = file.read();

        auto topology = frame.topology();

        CHECK(topology.size() == 65);

        CHECK(topology[0].type() == "Zn");
        CHECK(topology[1].type() == "O");

        CHECK(topology[0].name() == "ZN");
        CHECK(topology[1].name() == "O");

        CHECK(topology.bonds().size() == 68);

        CHECK(contains(topology.bonds(), Bond(9, 38)));
        CHECK(contains(topology.bonds(), Bond(58, 62)));
        CHECK(contains(topology.bonds(), Bond(37, 24)));
        CHECK(contains(topology.bonds(), Bond(27, 31)));

        CHECK(contains(topology.angles(), Angle(20, 21, 23)));
        CHECK(contains(topology.angles(), Angle(9, 38, 44)));

        CHECK(contains(topology.dihedrals(), Dihedral(64, 62, 58, 53)));
        CHECK(contains(topology.dihedrals(), Dihedral(22, 21, 23, 33)));
    }

    SECTION("Read frame properties") {
        auto file = Trajectory("data/pdb/2hkb.pdb");
        REQUIRE(file.nsteps() == 11);

        auto frame = file.read();
        CHECK(frame.get("classification")->as_string() == "DNA");
        CHECK(frame.get("deposition_date")->as_string() == "03-JUL-06");
        CHECK(frame.get("pdb_idcode")->as_string() == "2HKB");
        CHECK(frame.get("name")->as_string() ==
              "NMR STRUCTURE OF THE B-DNA DODECAMER CTCGGCGCCATC");

        file = Trajectory("data/pdb/4hhb.pdb");
        frame = file.read();
        CHECK(frame.get("classification")->as_string() == "OXYGEN TRANSPORT");
        CHECK(frame.get("deposition_date")->as_string() == "07-MAR-84");
        CHECK(frame.get("pdb_idcode")->as_string() == "4HHB");
        CHECK(frame.get("name")->as_string() ==
              "THE CRYSTAL STRUCTURE OF HUMAN DEOXYHAEMOGLOBIN AT 1.74 ANGSTROMS RESOLUTION");
    }

    SECTION("Read residue information") {
        auto file = Trajectory("data/pdb/water.pdb");
        auto frame = file.read();

        CHECK(frame.topology().residues().size() == 99);

        REQUIRE(frame.topology().residue_for_atom(1));
        auto residue = *frame.topology().residue_for_atom(1);
        CHECK(residue.size() == 3);
        CHECK(residue.contains(0));
        CHECK(residue.contains(1));
        CHECK(residue.contains(2));
        CHECK(residue.get("chainid"));
        CHECK(residue.get("chainid")->as_string() == "X");

        file = Trajectory("data/pdb/MOF-5.pdb");
        frame = file.read();

        CHECK(frame.topology().residues().size() == 1);
        residue = frame.topology().residues()[0];
        CHECK(residue.size() == frame.size());
        CHECK(residue.name() == "LIG");
    }

    SECTION("Read ATOM/HETATM information") {
        auto file = Trajectory("data/pdb/hemo.pdb");
        auto frame = file.read();
        auto residues = frame.topology().residues();

        // HEME group is first
        CHECK(residues[0].get("is_standard_pdb")->as_bool() == false);

        for (size_t i = 1; i < residues.size(); i++) {
            CHECK(residues[i].get("is_standard_pdb")->as_bool());
        }

        // Check to be use we've parsed the correct atom type
        CHECK(frame[74].mass() == 12.011);
    }

    SECTION("Handle multiple TER records") {
        auto file = Trajectory("data/pdb/4hhb.pdb");
        auto frame = file.read();

        CHECK(frame[4556].name() == "ND");
        CHECK(frame[4557].name() == "FE");
        CHECK(frame.topology().bond_order(4556, 4557) == 0);

        // The original behavior stored this, it is incorrect
        CHECK_THROWS(frame.topology().bond_order(4561, 4560));

        // Check secondary structure
        auto& topology = frame.topology();
        CHECK(topology.residue(05).get("secondary_structure")->as_string() == "alpha helix");
        CHECK(topology.residue(36).get("secondary_structure")->as_string() == "alpha helix");
    }

    SECTION("Secondary structure with insertion code test") {
        auto file = Trajectory("data/pdb/1bcu.pdb");
        auto frame = file.read();

        // Make sure the residues have been inserted correctly
        auto& topology = frame.topology();
        CHECK(topology.residue_for_atom(0)->name() == "ALA");
        CHECK(topology.residue_for_atom(0)->get("insertion_code")->as_string() == "B");
        CHECK(topology.residue_for_atom(5)->get("insertion_code")->as_string() == "A");
        CHECK_FALSE(topology.residue_for_atom(13)->get("insertion_code"));

        // Check secondary structure, no insertion code
        CHECK(topology.residue(9).get("secondary_structure")->as_string() == "3-10 helix");
        CHECK(topology.residue(10).get("secondary_structure")->as_string() == "3-10 helix");
        CHECK(topology.residue(11).get("secondary_structure")->as_string() == "3-10 helix");
        CHECK_FALSE(topology.residue(12).get("secondary_structure"));
        CHECK_FALSE(topology.residue(13).get("secondary_structure"));
        CHECK_FALSE(topology.residue(14).get("secondary_structure"));
        CHECK_FALSE(topology.residue(15).get("secondary_structure"));
        CHECK_FALSE(topology.residue(16).get("secondary_structure"));
        CHECK_FALSE(topology.residue(17).get("secondary_structure"));

        // First residue in a long list of residues with the same secondary structure
        auto& ins_check = topology.residue(18);
        CHECK(ins_check.get("secondary_structure")->as_string() == "alpha helix");
        CHECK(ins_check.get("insertion_code")->as_string() == "C");
        CHECK((*ins_check.id()) == 14);
        CHECK(ins_check.get("chainid")->as_string() == "L");

        CHECK(topology.residue(19).get("secondary_structure")->as_string() == "alpha helix");
        CHECK(topology.residue(19).get("insertion_code")->as_string() == "D");
        CHECK(topology.residue(20).get("secondary_structure")->as_string() == "alpha helix");
        CHECK(topology.residue(20).get("insertion_code")->as_string() == "E");
        CHECK(topology.residue(21).get("secondary_structure")->as_string() == "alpha helix");
        CHECK(topology.residue(21).get("insertion_code")->as_string() == "F");
        CHECK(topology.residue(22).get("secondary_structure")->as_string() == "alpha helix");
        CHECK(topology.residue(22).get("insertion_code")->as_string() == "G");

        // Not included
        CHECK_FALSE(topology.residue(23).get("secondary_structure"));
        CHECK(topology.residue(23).get("insertion_code")->as_string() == "H");
        CHECK((*topology.residue(23).id()) == 14);
        CHECK(topology.residue(23).get("chainid")->as_string() == "L");
    }

    SECTION("Read Protein Residues") {
        auto frame = Trajectory("data/pdb/hemo.pdb").read();
        auto topology = frame.topology();

        CHECK(!topology.are_linked(topology.residue(2), topology.residue(3)));
        CHECK( topology.are_linked(topology.residue(3), topology.residue(4)));
        CHECK(!topology.are_linked(topology.residue(3), topology.residue(5)));
        CHECK(topology.bonds().size() == 482);
    }

    SECTION("Read Nucleic Residues") {
        auto frame = Trajectory("data/pdb/2hkb.pdb").read();
        auto topology = frame.topology();

        CHECK(topology.are_linked(topology.residue(3), topology.residue(4)));
        CHECK(!topology.are_linked(topology.residue(3), topology.residue(5)));
        CHECK(topology.bonds().size() == 815);
    }

    SECTION("Read atomic insertion codes") {
        auto frame = Trajectory("data/pdb/insertion-code.pdb").read();
        auto& topology = frame.topology();

        CHECK(topology.residue_for_atom(0)->get("insertion_code")->as_string() == "a");
        CHECK(topology.residue_for_atom(1)->get("insertion_code")->as_string() == "c");
        CHECK(topology.residue_for_atom(2)->get("insertion_code")->as_string() == "x");
        CHECK_FALSE(frame[3].get("insertion_code"));
    }
}

TEST_CASE("Problematic PDB files") {
    SECTION("Odd PDB numbering") {
        auto file = Trajectory("data/pdb/odd-start.pdb");
        auto frame = file.read();

        REQUIRE(frame.size() == 20);
        CHECK(frame[0].name() == "C1");
        CHECK(frame[19].name() == "C18");
        CHECK(frame.topology().bond_order(0, 1) == 0);
        CHECK(frame.topology().bond_order(19, 13) == 0);
    }

    SECTION("Atom ID starts at 0") {
        auto file = Trajectory("data/pdb/atom-id-0.pdb");
        REQUIRE(file.nsteps() == 1);
        auto frame = file.read();
        REQUIRE(frame.size() == 2);

        CHECK(frame[0].name() == "C1");
        CHECK(frame[1].name() == "C2");
        CHECK(frame[0].type() == "C");
        CHECK(frame[1].type() == "C");

        CHECK(approx_eq(frame.positions()[0], {0.867, 53.138, 165.015}, 1e-5));
        CHECK(approx_eq(frame.positions()[1], {-4.186, 51.104, 163.241}, 1e-5));
    }

    SECTION("Multiple END records") {
        auto file = Trajectory("data/pdb/end-endmdl.pdb");
        REQUIRE(file.nsteps() == 2);

        auto frame = file.read();
        REQUIRE(frame.size() == 4);

        frame = file.read();
        REQUIRE(frame.size() == 7);
    }

    SECTION("Multiple MODEL without END") {
        auto file = Trajectory("data/pdb/model.pdb");
        REQUIRE(file.nsteps() == 2);

        auto frame = file.read();
        REQUIRE(frame.size() == 2223);

        frame = file.read();
        REQUIRE(frame.size() == 2223);
    }

    SECTION("File generated by Crystal Maker") {
        auto file = Trajectory("data/pdb/crystal-maker.pdb");
        REQUIRE(file.nsteps() == 1);

        auto frame = file.read();
        REQUIRE(frame.size() == 8);
    }

    SECTION("Short CRYST1 record") {
        auto file = Trajectory("data/pdb/short-cryst1.pdb");
        auto frame = file.read();
    }

    SECTION("Short ATOM record") {
        auto file = Trajectory("data/pdb/short-atom.pdb");
        auto frame = file.read();
        REQUIRE(frame.size() == 9);

        CHECK(frame[0].name() == "O");
        CHECK(frame[5].name() == "H");
        CHECK(frame[0].type() == "O");
        CHECK(frame[5].type() == "H");

        CHECK(approx_eq(frame.positions()[0], {0.417, 8.303, 11.737}, 1e-5));
        CHECK(approx_eq(frame.positions()[5], {8.922, 9.426, 5.320}, 1e-5));
    }

    SECTION("Bug in 1HTQ") {
        // https://github.com/chemfiles/chemfiles/issues/328
        // some secondary structure residues are not in the expected order
        auto file = Trajectory("data/pdb/1htq.pdb");
        auto frame = file.read();
    }

    SECTION("File generated by ASE") {
        // The file is a bit strange already, and had a bug where not all steps
        // would be found. The bug camed from an interaction with buffering of
        // files, and the usage of string_view in TextFile::readline().
        auto file = Trajectory("data/pdb/ase.pdb");
        REQUIRE(file.nsteps() == 156);
    }
}

TEST_CASE("Write files in PDB format") {
    auto tmpfile = NamedTempPath(".pdb");
    const auto EXPECTED_CONTENT =
    "MODEL    1\n"
    "CRYST1   22.000   22.000   22.000  90.00  90.00  90.00 P 1           1\n"
    "HETATM    1 A   AXXX X   1       1.000   2.000   3.000  1.00  0.00           A\n"
    "HETATM    2 B   BXXX X   2       1.000   2.000   3.000  1.00  0.00           B\n"
    "HETATM    3 C    XXX X   3       1.000   2.000   3.000  1.00  0.00           C\n"
    "HETATM    4 D    XXX X   4       1.000   2.000   3.000  1.00  0.00           D\n"
    "CONECT    1    2\n"
    "CONECT    2    1\n"
    "ENDMDL\n"
    "MODEL    2\n"
    "CRYST1   22.000   22.000   22.000  90.00  90.00  90.00 P 1           1\n"
    "HETATM    1 A   AXXX X   4       1.000   2.000   3.000  1.00  0.00           A\n"
    "ATOM      2 B   Bfoo A   3       1.000   2.000   3.000  1.00  0.00           B\n"
    "ATOM      3 C    foo A   3       1.000   2.000   3.000  1.00  0.00           C\n"
    "TER       4      foo A   3 \n"
    "HETATM    5 D    bar C  -1B      1.000   2.000   3.000  1.00  0.00           D\n"
    "HETATM    6 E    XXX X   5       4.000   5.000   6.000  1.00  0.00           E\n"
    "HETATM    7 F    baz X  -2       4.000   5.000   6.000  1.00  0.00           F\n"
    "HETATM    8 G    XXX X   6       4.000   5.000   6.000  1.00  0.00           G\n"
    "CONECT    1    2    8\n"
    "CONECT    2    1    8\n"
    "CONECT    3    8\n"
    "CONECT    5    8\n"
    "CONECT    6    7    8\n"
    "CONECT    7    6    8\n"
    "CONECT    8    1    2    3    5\n"
    "CONECT    8    6    7\n"
    "ENDMDL\n"
    "END\n";

    auto frame = Frame(UnitCell(22));
    frame.add_atom(Atom("A"), {1, 2, 3});
    frame.add_atom(Atom("B"), {1, 2, 3});
    frame.add_atom(Atom("C"), {1, 2, 3});
    frame.add_atom(Atom("D"), {1, 2, 3});
    frame.add_bond(0, 1);
    frame[0].set("altloc", "A");
    frame[1].set("altloc", "BB");

    auto file = Trajectory(tmpfile, 'w');
    file.write(frame);

    frame.add_atom(Atom("E"), {4, 5, 6});
    frame.add_atom(Atom("F"), {4, 5, 6});
    frame.add_atom(Atom("G"), {4, 5, 6});
    frame.add_bond(4, 5);
    frame.add_bond(0, 6);
    frame.add_bond(1, 6);
    frame.add_bond(1, 2); // This bond will not be printed
    frame.add_bond(2, 6);
    frame.add_bond(3, 6);
    frame.add_bond(4, 6);
    frame.add_bond(5, 6);

    Residue residue("foo", 3);
    residue.add_atom(1);
    residue.add_atom(2);
    residue.set("chainid", "A");
    residue.set("is_standard_pdb", true);
    residue.set("composition_type", "L-PEPTIDE LINKING");
    frame.add_residue(residue);

    residue = Residue("barbar"); // This will be truncated in output
    residue.add_atom(3);
    residue.set("chainid", "CB");
    residue.set("insertion_code", "BB");
    frame.add_residue(residue);

    residue = Residue("baz", -2);
    residue.add_atom(5);
    frame.add_residue(residue);

    file.write(frame);
    file.close();

    auto check_pdb = Trajectory(tmpfile);
    CHECK(check_pdb.nsteps() == 2);
    auto frame1 = check_pdb.read();
    CHECK(frame1.size() == 4);
    CHECK(frame1[0].get("altloc")->as_string() == "A");
    CHECK(frame1[1].get("altloc")->as_string() == "B");
    CHECK(check_pdb.read().size() == 7);
    check_pdb.close();

    std::ifstream checking(tmpfile);
    std::string content((std::istreambuf_iterator<char>(checking)),
                         std::istreambuf_iterator<char>());
    CHECK(content == EXPECTED_CONTENT);
}

TEST_CASE("PDB files with big values") {
    SECTION("Unit cell and Coordinates") {
        auto tmpfile = NamedTempPath(".pdb");
        auto trajectory = Trajectory(tmpfile, 'w');

        auto frame = Frame();
        frame.resize(1);
        frame.set_cell(UnitCell(1234567890));
        CHECK_THROWS_AS(trajectory.write(frame), FormatError);

        frame.set_cell(UnitCell(12));
        frame.positions()[0] = Vector3D(123456789, 2, 3);
        CHECK_THROWS_AS(trajectory.write(frame), FormatError);
    }

    SECTION("Default residues") {
        auto tmpfile = NamedTempPath(".pdb");

        auto frame = Frame();
        for(size_t i=0; i<10001; i++) {
            frame.add_atom(Atom("A"), {0, 0, 0});
        }
        auto positions = frame.positions();
        positions[998] = Vector3D(1., 2., 3.);
        positions[9998] = Vector3D(4., 5., 6.);
        positions[9999] = Vector3D(7., 8., 9.);

        Trajectory(tmpfile, 'w').write(frame);

        // Re-read the file we just wrote
        frame = Trajectory(tmpfile, 'r').read();
        positions = frame.positions();

        // If resSeq is has more than 4 characters, coordinates won't be read
        // correctly
        CHECK(positions[998] == Vector3D(1., 2., 3.));
        CHECK(positions[9998] == Vector3D(4., 5., 6.));
        CHECK(positions[9999] == Vector3D(7., 8., 9.));
    }

    SECTION("User specified residues") {
        auto tmpfile = NamedTempPath(".pdb");

        auto frame = Frame();
        for(size_t i=0; i<10001; i++) {
            frame.add_atom(Atom("A"), {0, 0, 0});
            Residue residue("ANA", i + 1);
            residue.add_atom(i);
            frame.add_residue(std::move(residue));
        }
        auto positions = frame.positions();
        positions[998] = Vector3D(1., 2., 3.);
        positions[9998] = Vector3D(4., 5., 6.);
        positions[9999] = Vector3D(7., 8., 9.);

        Trajectory(tmpfile, 'w').write(frame);

        // Re-read the file we just wrote
        frame = Trajectory(tmpfile, 'r').read();
        positions = frame.positions();

        // If resSeq is has more than 4 characters, coordinates won't be read
        // correctly
        CHECK(positions[998] == Vector3D(1., 2., 3.));
        CHECK(positions[9998] == Vector3D(4., 5., 6.));
        CHECK(positions[9999] == Vector3D(7., 8., 9.));
    }

    SECTION("CONNECT with too many atoms") {
        if (!is_valgrind_and_travis()) {
            auto tmpfile = NamedTempPath(".pdb");

            auto frame = Frame();
            for(size_t i=0; i<110000; i++) {
                frame.add_atom(Atom("A"), {0.0, 0.0, 0.0});
            }
            frame.add_bond(101000, 101008);
            Trajectory(tmpfile, 'w').write(frame);

            // Re-read the file we just wrote
            frame = Trajectory(tmpfile, 'r').read();
            CHECK(frame.topology().bonds().empty());
        }
    }
}

TEST_CASE("Read and write files in memory") {
    SECTION("Reading from memory") {
        std::ifstream checking("data/pdb/water.pdb");
        std::vector<char> content((std::istreambuf_iterator<char>(checking)),
            std::istreambuf_iterator<char>());

        auto file = Trajectory::memory_reader(content.data(), content.size(), "PDB");
        REQUIRE(file.nsteps() == 100);

        Frame frame = file.read();

        REQUIRE(frame.size() == 297);
        auto positions = frame.positions();
        CHECK(approx_eq(positions[0], Vector3D(0.417, 8.303, 11.737), 1e-3));
        CHECK(approx_eq(positions[296], Vector3D(6.664, 11.6148, 12.961), 1e-3));

        auto cell = frame.cell();
        CHECK(cell.shape() == UnitCell::ORTHORHOMBIC);
        CHECK(approx_eq(cell.a(), 15.0, 1e-5));

        file.read(); // Skip a frame
        frame = file.read();

        REQUIRE(frame.size() == 297);
        positions = frame.positions();
        CHECK(approx_eq(positions[0], Vector3D(0.299, 8.310, 11.721), 1e-4));
        CHECK(approx_eq(positions[296], Vector3D(6.798, 11.509, 12.704), 1e-4));
    }
}
