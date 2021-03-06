#define HERMES_REPORT_ALL
#define HERMES_REPORT_FILE "application.log"
#include "definitions.h"

using namespace RefinementSelectors;

//  This example is a continuation of the example "09-timedep-basic" and it shows how 
//  to perform time integration with arbitrary Runge-Kutta methods, using Butcher's 
//  tables as input parameters. Currently (as of January 2011) approx. 30 tables are 
//  available by default, as can be seen below. They are taken from various sources 
//  including J. Butcher's book, journal articles, and the Wikipedia page 
//  http://en.wikipedia.org/wiki/List_of_Runge%E2%80%93Kutta_methods. If you know 
//  about some other interesting R-K method that we are missing here, please let us 
//  know!
//
//  PDE: non-stationary heat transfer equation
//       HEATCAP * RHO * dT/dt - LAMBDA * Laplace T = 0.
//  This equation is, however, written in such a way that the time-derivative 
//  is on the left and everything else on the right:
//
//  dT/dt = LAMBDA * Laplace T / (HEATCAP * RHO).
//
//  As opposed to the previous example where the time-discretization was hardwired
//  into the weak formulation, now we only need the weak formulation of the 
//  right-hand side.
//
//  Domain: St. Vitus cathedral (file cathedral.mesh).
//
//  IC:  T = TEMP_INIT.
//  BC:  T = TEMP_INIT on the bottom edge ... Dirichlet,
//       LAMBDA * dT/dn = ALPHA*(t_exterior(time) - T) ... Newton, time-dependent.
//
//  Time-stepping: Arbitrary Runge-Kutta methods.
//
//  The following parameters can be changed:

// Polynomial degree of mesh elements.
const int P_INIT = 2;                             
// Number of initial uniform mesh refinements.
const int INIT_REF_NUM = 1;                       
// Number of initial uniform mesh refinements towards the boundary.
const int INIT_REF_NUM_BDY = 3;                   
// Time step in seconds.
const double time_step = 3e+2;                    
 // Stopping criterion for the Newton's method.
const double NEWTON_TOL = 1e-5;                  
// Maximum allowed number of Newton iterations.
const int NEWTON_MAX_ITER = 100;                  
// Matrix solver: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
// SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  

// Choose one of the following time-integration methods, or define your own Butcher's table. The last number 
// in the name of each method is its order. The one before last, if present, is the number of stages.
// Explicit methods:
//   Explicit_RK_1, Explicit_RK_2, Explicit_RK_3, Explicit_RK_4.
// Implicit methods: 
//   Implicit_RK_1, Implicit_Crank_Nicolson_2_2, Implicit_SIRK_2_2, Implicit_ESIRK_2_2, Implicit_SDIRK_2_2, 
//   Implicit_Lobatto_IIIA_2_2, Implicit_Lobatto_IIIB_2_2, Implicit_Lobatto_IIIC_2_2, Implicit_Lobatto_IIIA_3_4, 
//   Implicit_Lobatto_IIIB_3_4, Implicit_Lobatto_IIIC_3_4, Implicit_Radau_IIA_3_5, Implicit_SDIRK_5_4.
// Embedded explicit methods:
//   Explicit_HEUN_EULER_2_12_embedded, Explicit_BOGACKI_SHAMPINE_4_23_embedded, Explicit_FEHLBERG_6_45_embedded,
//   Explicit_CASH_KARP_6_45_embedded, Explicit_DORMAND_PRINCE_7_45_embedded.
// Embedded implicit methods:
//   Implicit_SDIRK_CASH_3_23_embedded, Implicit_ESDIRK_TRBDF2_3_23_embedded, Implicit_ESDIRK_TRX2_3_23_embedded, 
//   Implicit_SDIRK_BILLINGTON_3_23_embedded, Implicit_SDIRK_CASH_5_24_embedded, Implicit_SDIRK_CASH_5_34_embedded, 
//   Implicit_DIRK_ISMAIL_7_45_embedded. 
ButcherTableType butcher_table_type = Implicit_SDIRK_2_2;

// Problem parameters.
// Temperature of the ground (also initial temperature).
const double TEMP_INIT = 10;       
// Heat flux coefficient for Newton's boundary condition.
const double ALPHA = 10;           
// Thermal conductivity of the material.
const double LAMBDA = 1e2;         
// Heat capacity.
const double HEATCAP = 1e2;        
// Material density.
const double RHO = 3000;           
// Length of time interval (24 hours) in seconds.
const double T_FINAL = 86400;      

int main(int argc, char* argv[])
{
  // Choose a Butcher's table or define your own.
  ButcherTable bt(butcher_table_type);
  if (bt.is_explicit()) info("Using a %d-stage explicit R-K method.", bt.get_size());
  if (bt.is_diagonally_implicit()) info("Using a %d-stage diagonally implicit R-K method.", bt.get_size());
  if (bt.is_fully_implicit()) info("Using a %d-stage fully implicit R-K method.", bt.get_size());

  // Load the mesh.
  Mesh mesh;
  MeshReaderH2D mloader;
  mloader.load("domain.mesh", &mesh);

  // Perform initial mesh refinements.
  for(int i = 0; i < INIT_REF_NUM; i++) mesh.refine_all_elements();
  mesh.refine_towards_boundary("Boundary_air", INIT_REF_NUM_BDY);
  mesh.refine_towards_boundary("Boundary_ground", INIT_REF_NUM_BDY);

  // Previous and next time level solutions.
  CustomInitialCondition sln_time_prev(&mesh, TEMP_INIT);
  Solution<double> sln_time_new(&mesh);

  // Initialize the weak formulation.
  double current_time = 0;

  CustomWeakFormHeatRK wf("Boundary_air", ALPHA, LAMBDA, HEATCAP, RHO, 
                          &current_time, TEMP_INIT, T_FINAL);
  
  // Initialize boundary conditions.
  DefaultEssentialBCConst<double> bc_essential("Boundary_ground", TEMP_INIT);
  EssentialBCs<double> bcs(&bc_essential);

  // Create an H1 space with default shapeset.
  H1Space<double> space(&mesh, &bcs, P_INIT);
  int ndof = space.get_num_dofs();
  info("ndof = %d", ndof);

  // Initialize the FE problem.
  DiscreteProblem<double> dp(&wf, &space);

  // Initialize views.
  ScalarView Tview("Temperature", new WinGeom(0, 0, 450, 600));
  Tview.set_min_max_range(0,20);
  Tview.fix_scale_width(30);

  // Initialize Runge-Kutta time stepping.
  RungeKutta<double> runge_kutta(&dp, &bt, matrix_solver);

  // Time stepping loop:
  int ts = 1;
  do 
  {
    // Perform one Runge-Kutta time step according to the selected Butcher's table.
    info("Runge-Kutta time step (t = %g s, time step = %g s, stages: %d).", 
         current_time, time_step, bt.get_size());
    bool freeze_jacobian = false;
    bool block_diagonal_jacobian = false;
    bool verbose = true;
    double damping_coeff = 1.0;
    double max_allowed_residual_norm = 1e10;
    try
    {
      runge_kutta.rk_time_step_newton(current_time, time_step, &sln_time_prev, 
                                  &sln_time_new, freeze_jacobian, block_diagonal_jacobian, verbose,
                                  NEWTON_TOL, NEWTON_MAX_ITER, damping_coeff,
                                  max_allowed_residual_norm);
    }
    catch(Exceptions::Exception& e)
    {
      e.printMsg();
      error("Runge-Kutta time step failed");
    }

    // Show the new time level solution.
    char title[100];
    sprintf(title, "Time %3.2f s", current_time);
    Tview.set_title(title);
    Tview.show(&sln_time_new);

    // Copy solution for the new time step.
    sln_time_prev.copy(&sln_time_new);

    // Increase current time and time step counter.
    current_time += time_step;
    ts++;
  } 
  while (current_time < T_FINAL);

  // Wait for the view to be closed.
  View::wait();
  return 0;
}
