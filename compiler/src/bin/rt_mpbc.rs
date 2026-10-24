use std::fs;
use std::path::Path;
use rtmpbc_compiler::*;

fn main() {
    println!("============================================================");
    println!(" RT-MPBC: Real-Time Multi-Physics Bond-Graph Compiler v1.0 ");
    println!(" Zero-Allocation Bare-Metal Symplectic Code Generator      ");
    println!("============================================================");

    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        println!("Usage: rt-mpbc <command> [options]");
        println!("Commands:");
        println!("  demo-ev          Run the EV Powertrain Multi-Physics Compiler Demo");
        println!("  demo-muscle      Run the Biomechanical Muscle-Tendon Actuator Demo");
        println!("  demo-robot       Run the Robotic Joint Virtual Sensor Demo");
        println!("  compile <file>   Compile JSON bond-graph model to C and Rust");
        return;
    }

    match args[1].as_str() {
        "demo-ev" => run_ev_demo(),
        "demo-muscle" => run_muscle_demo(),
        "demo-robot" => run_robot_demo(),
        _ => {
            println!("Unknown command: {}", args[1]);
        }
    }
}

fn run_ev_demo() {
    println!("\n[1] Constructing Electric Vehicle (EV) Powertrain Multi-Physics Graph...");
    let mut bg = BondGraph::new("ev_powertrain");

    // Electrical Subsystem
    let n_bat = bg.add_node(ElementType::C { name: "C_Battery".into(), value: 100.0 });
    let n_j0_elec = bg.add_node(ElementType::Junction0 { name: "J0_Bus".into() });
    let n_r_int = bg.add_node(ElementType::R { name: "R_Battery_Int".into(), value: 0.05 });
    let n_gy_motor = bg.add_node(ElementType::GY { name: "GY_PMSM_Kt".into(), modulus: 0.85 });

    // Mechanical Subsystem
    let n_j1_rotor = bg.add_node(ElementType::Junction1 { name: "J1_Rotor".into() });
    let n_i_rotor = bg.add_node(ElementType::I { name: "I_Rotor_Inertia".into(), value: 0.02 });
    let n_tf_gear = bg.add_node(ElementType::TF { name: "TF_Gearbox".into(), modulus: 9.5 });
    let n_j1_axle = bg.add_node(ElementType::Junction1 { name: "J1_Axle".into() });
    let n_c_shaft = bg.add_node(ElementType::C { name: "C_Shaft_Torsion".into(), value: 0.001 });
    let n_i_vehicle = bg.add_node(ElementType::I { name: "I_Vehicle_Mass".into(), value: 1800.0 });
    let n_r_drag = bg.add_node(ElementType::R { name: "R_Aero_Rolling".into(), value: 25.0 });

    // Connections
    bg.connect(n_bat, n_j0_elec, Domain::Electrical);
    bg.connect(n_j0_elec, n_r_int, Domain::Electrical);
    bg.connect(n_j0_elec, n_gy_motor, Domain::Electrical);

    bg.connect(n_gy_motor, n_j1_rotor, Domain::MechanicalRotational);
    bg.connect(n_j1_rotor, n_i_rotor, Domain::MechanicalRotational);
    bg.connect(n_j1_rotor, n_tf_gear, Domain::MechanicalRotational);

    bg.connect(n_tf_gear, n_j1_axle, Domain::MechanicalRotational);
    bg.connect(n_j1_axle, n_c_shaft, Domain::MechanicalRotational);
    bg.connect(n_j1_axle, n_i_vehicle, Domain::MechanicalTranslational);
    bg.connect(n_j1_axle, n_r_drag, Domain::MechanicalTranslational);

    println!("  Total nodes: {}, Total bonds: {}", bg.nodes.len(), bg.bonds.len());

    println!("\n[2] Executing Sequential Causality Assignment Procedure (SCAP)...");
    let mut scap_solver = ScapSolver::new(&mut bg);
    let scap_result = scap_solver.solve();

    println!("  SCAP Status: Success = {}", scap_result.success);
    println!("  Independent States: {}", scap_result.num_independent_states);
    for &idx in &scap_result.independent_state_nodes {
        println!("    - State: {}", bg.nodes[idx].name());
    }
    println!("  Dependent States (Derivative Causality): {}", scap_result.dependent_state_nodes.len());
    for &idx in &scap_result.dependent_state_nodes {
        println!("    - Collapsed via Coordinate Reduction: {}", bg.nodes[idx].name());
    }

    println!("\n[3] Synthesizing Skew-Symmetric Dirac Interconnection Matrices...");
    let synthesizer = DiracSynthesizer::new(&bg, &scap_result);
    let matrices = synthesizer.synthesize();

    println!("  Synthesized {}x{} J (lossless) and R (dissipative) matrices", matrices.n_states, matrices.n_states);

    println!("\n[4] Unrolling Symbolic Sparse Instruction Tape (Zero-Branching)...");
    let dt = 0.0001; // 10 kHz
    let unroller = SparseUnroller::new(&matrices, dt);
    let tape = unroller.compile_tape();
    println!("  Generated flat tape with {} arithmetic operations", tape.ops.len());

    println!("\n[5] Emitting Production C99 and Bare-Metal Rust Code...");
    let codegen_c = CodeGenC::new("ev_powertrain", &matrices, &tape, dt);
    let c_header = codegen_c.emit_header();
    let c_source = codegen_c.emit_source();

    let out_dir = Path::new("examples/01_ev_powertrain");
    fs::create_dir_all(out_dir).expect("Failed to create dir");
    fs::write(out_dir.join("ev_powertrain.h"), c_header).expect("Failed to write h");
    fs::write(out_dir.join("ev_powertrain.c"), c_source).expect("Failed to write c");

    let codegen_rust = CodeGenRust::new("ev_powertrain", &matrices, &tape, dt);
    let rust_code = codegen_rust.emit_code();
    fs::write(out_dir.join("ev_powertrain.rs"), rust_code).expect("Failed to write rs");

    println!("  Successfully emitted:\n    - {}\n    - {}\n    - {}",
        out_dir.join("ev_powertrain.h").display(),
        out_dir.join("ev_powertrain.c").display(),
        out_dir.join("ev_powertrain.rs").display()
    );
    println!("\n[DONE] EV Powertrain Multi-Physics Compiler Pipeline Complete!");
}

fn run_muscle_demo() {
    println!("\n[1] Constructing Biomechanical Muscle-Tendon Multi-Physics Graph...");
    let mut bg = BondGraph::new("fhe_muscle_actuator");

    // Electrical Activation (Cole-Cole bio-impedance)
    let n_vin = bg.add_node(ElementType::Se { name: "V_Stimulation".into(), value: 45.0 });
    let n_j0_skin = bg.add_node(ElementType::Junction0 { name: "J0_Skin_Electrode".into() });
    let n_r_skin = bg.add_node(ElementType::R { name: "R_Stratum_Corneum".into(), value: 1200.0 });
    let n_c_skin = bg.add_node(ElementType::C { name: "C_Epidermal_Membrane".into(), value: 1e-6 });

    // Electromechanical Coupling (Actin-Myosin Cross-Bridge)
    let n_gy_muscle = bg.add_node(ElementType::GY { name: "GY_CrossBridge".into(), modulus: 12.0 });

    // Viscoelastic Tendon (Kelvin-Voigt / Maxwell dynamic tissue compliance & damping)
    let n_j1_tendon = bg.add_node(ElementType::Junction1 { name: "J1_Tendon_Node".into() });
    let n_c_tendon = bg.add_node(ElementType::C { name: "C_Tendon_Elasticity".into(), value: 0.0002 });
    let n_r_viscous = bg.add_node(ElementType::R { name: "R_Fascial_Damping".into(), value: 35.0 });
    let n_i_limb = bg.add_node(ElementType::I { name: "I_Forearm_Inertia".into(), value: 0.15 });

    bg.connect(n_vin, n_j0_skin, Domain::Electrical);
    bg.connect(n_j0_skin, n_r_skin, Domain::Electrical);
    bg.connect(n_j0_skin, n_c_skin, Domain::Electrical);
    bg.connect(n_j0_skin, n_gy_muscle, Domain::Electrical);

    bg.connect(n_gy_muscle, n_j1_tendon, Domain::MechanicalTranslational);
    bg.connect(n_j1_tendon, n_c_tendon, Domain::MechanicalTranslational);
    bg.connect(n_j1_tendon, n_r_viscous, Domain::MechanicalTranslational);
    bg.connect(n_j1_tendon, n_i_limb, Domain::MechanicalTranslational);

    let mut scap = ScapSolver::new(&mut bg);
    let scap_res = scap.solve();
    let syn = DiracSynthesizer::new(&bg, &scap_res);
    let mat = syn.synthesize();

    let dt = 0.0005; // 2 kHz
    let unroller = SparseUnroller::new(&mat, dt);
    let tape = unroller.compile_tape();

    let out_dir = Path::new("examples/02_fhe_muscle_actuator");
    fs::create_dir_all(out_dir).expect("Failed to create dir");
    fs::write(out_dir.join("muscle_actuator.h"), CodeGenC::new("muscle_actuator", &mat, &tape, dt).emit_header()).unwrap();
    fs::write(out_dir.join("muscle_actuator.c"), CodeGenC::new("muscle_actuator", &mat, &tape, dt).emit_source()).unwrap();
    fs::write(out_dir.join("muscle_actuator.rs"), CodeGenRust::new("muscle_actuator", &mat, &tape, dt).emit_code()).unwrap();

    println!("  Successfully compiled and emitted Biomechanical Muscle-Tendon Actuator Model!");
}

fn run_robot_demo() {
    println!("\n[1] Constructing Robotic Joint Virtual Sensor Multi-Physics Graph...");
    let mut bg = BondGraph::new("robotic_virtual_sensor");

    let n_vbus = bg.add_node(ElementType::Se { name: "V_PWM_Inverter".into(), value: 48.0 });
    let n_j1_elec = bg.add_node(ElementType::Junction1 { name: "J1_Phase_Arm".into() });
    let n_r_phase = bg.add_node(ElementType::R { name: "R_Winding_Copper".into(), value: 0.12 });
    let n_i_phase = bg.add_node(ElementType::I { name: "I_Winding_Inductance".into(), value: 0.00045 });

    let n_gy_emf = bg.add_node(ElementType::GY { name: "GY_Motor_Ke".into(), modulus: 0.15 });
    let n_j1_mech = bg.add_node(ElementType::Junction1 { name: "J1_Harmonic_Drive".into() });
    let n_c_flex = bg.add_node(ElementType::C { name: "C_Gear_Flexspline".into(), value: 0.00005 });
    let n_r_bear = bg.add_node(ElementType::R { name: "R_Bearing_Friction".into(), value: 0.015 });
    let n_i_load = bg.add_node(ElementType::I { name: "I_Link_Payload".into(), value: 0.8 });

    bg.connect(n_vbus, n_j1_elec, Domain::Electrical);
    bg.connect(n_j1_elec, n_r_phase, Domain::Electrical);
    bg.connect(n_j1_elec, n_i_phase, Domain::Electrical);
    bg.connect(n_j1_elec, n_gy_emf, Domain::Electrical);

    bg.connect(n_gy_emf, n_j1_mech, Domain::MechanicalRotational);
    bg.connect(n_j1_mech, n_c_flex, Domain::MechanicalRotational);
    bg.connect(n_j1_mech, n_r_bear, Domain::MechanicalRotational);
    bg.connect(n_j1_mech, n_i_load, Domain::MechanicalRotational);

    let mut scap = ScapSolver::new(&mut bg);
    let scap_res = scap.solve();
    let syn = DiracSynthesizer::new(&bg, &scap_res);
    let mat = syn.synthesize();

    let dt = 0.00005; // 20 kHz
    let unroller = SparseUnroller::new(&mat, dt);
    let tape = unroller.compile_tape();

    let out_dir = Path::new("examples/03_robotic_virtual_sensor");
    fs::create_dir_all(out_dir).expect("Failed to create dir");
    fs::write(out_dir.join("robotic_sensor.h"), CodeGenC::new("robotic_sensor", &mat, &tape, dt).emit_header()).unwrap();
    fs::write(out_dir.join("robotic_sensor.c"), CodeGenC::new("robotic_sensor", &mat, &tape, dt).emit_source()).unwrap();
    fs::write(out_dir.join("robotic_sensor.rs"), CodeGenRust::new("robotic_sensor", &mat, &tape, dt).emit_code()).unwrap();

    println!("  Successfully compiled and emitted Robotic Joint Virtual Sensor Model!");
}
