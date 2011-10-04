Complex-Valued Problem (04-complex)
-----------------------------------

This example shows how to define complex-valued weak forms, essential boundary conditions,
spaces, solutions, discrete problem, and how to perform orthogonal projection and adaptivity 
in complex mode. In addition, it shows how to use the AZTECOO matrix solver (other solvers
including UMFPACK can be used as well).

Model problem
~~~~~~~~~~~~~

We solve a complex-valued vector potential problem

.. math::

    -\Delta A + j \omega \gamma \mu A - \mu J_{ext} = 0

in a two-dimensional cross-section containing a conductor and an iron object.
Note: in 2D this is a scalar problem. A sketch of the computational domain 
is shown in the following picture:

.. figure:: 04-complex/domain.png
   :align: center
   :scale: 80% 
   :figclass: align-center
   :alt: Domain.

The computational domain is a rectangle of height 0.003 and width 0.004. 
Different material markers are used for the wire, air, and iron 
(see mesh file "domain.mesh").

Boundary conditions are zero Dirichlet on the top and right edges, and zero Neumann
elsewhere.

Complex-valued weak forms
~~~~~~~~~~~~~~~~~~~~~~~~~

The weak formulation consists entirely of default forms, it's header looks as follows::

    class CustomWeakForm : public WeakForm<std::complex<double> >
    { 
    public:
      CustomWeakForm(std::string mat_air,  double mu_air,
		     std::string mat_iron, double mu_iron, double gamma_iron,
		     std::string mat_wire, double mu_wire, std::complex<double> j_ext, double omega);
    };

For details see the files definitions.h and definitions.cpp.

Initializing the AztecOO matrix solver
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The matrix solver is initialized as usual::

    // Initialize matrix solver.
    initialize_solution_environment(matrix_solver, argc, argv);
    SparseMatrix* matrix = create_matrix(matrix_solver);
    Vector* rhs = create_vector(matrix_solver);
    Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);

When using AztecOO, we need to select an iterative method and preconditioner::

    if (matrix_solver == SOLVER_AZTECOO) {
      ((AztecOOSolver*) solver)->set_solver(iterative_method);
      ((AztecOOSolver*) solver)->set_precond(preconditioner);
      // Using default iteration parameters (see solver/aztecoo.h).
    }

Here, "iterative_method" and "preconditioner" have been defined at the 
beginning of the file main.cpp as

.. sourcecode::
    .

    const char* iterative_method = "gmres";           // Name of the iterative method employed by AztecOO (ignored
                                                      // by the other solvers). 
                                                      // Possibilities: gmres, cg, cgs, tfqmr, bicgstab.
    const char* preconditioner = "least-squares";     // Name of the preconditioner employed by AztecOO (ignored by
                                                      // the other solvers).
                                                      // Possibilities: none, jacobi, neumann, least-squares, or a
                                                      // preconditioner from IFPACK (see solver/aztecoo.h)

.. latexcode::
    .

    const char* iterative_method = "gmres";           // Name of the iterative method
                                                      // employed by AztecOO (ignored
                                                      // by the other solvers). 
                                                      // Possibilities: gmres, cg, cgs,
                                                      // tfqmr, bicgstab.
    
    const char* preconditioner = "least-squares";     // Name of the preconditioner
                                                      // employed by AztecOO (ignored by
                                                      // the other solvers).
                                                      // Possibilities: none, jacobi,
                                                      // neumann, least-squares, or a
                                                      // preconditioner from IFPACK 
                                                      // (see solver/aztecoo.h)

Otherwise everything works in the same way as in example 
"01-intro".

Sample results
~~~~~~~~~~~~~~

Solution:

.. figure:: 04-complex/solution.png
   :align: center
   :scale: 50% 
   :figclass: align-center
   :alt: Solution.

Let us compare adaptive $h$-FEM with linear and quadratic elements and the $hp$-FEM.

Final mesh for $h$-FEM with linear elements: 18694 DOF, error = 1.02 \%

.. figure:: 04-complex/mesh-h1.png
   :align: center
   :scale: 45% 
   :figclass: align-center
   :alt: Mesh.

Final mesh for $h$-FEM with quadratic elements: 46038 DOF, error = 0.018 \%

.. figure:: 04-complex/mesh-h2.png
   :align: center
   :scale: 45% 
   :figclass: align-center
   :alt: Mesh.

Final mesh for $hp$-FEM: 4787 DOF, error = 0.00918 \%

.. figure:: 04-complex/mesh-hp.png
   :align: center
   :scale: 45% 
   :figclass: align-center
   :alt: Mesh.

Convergence graphs of adaptive h-FEM with linear elements, h-FEM with quadratic elements
and hp-FEM are shown below.

.. figure:: 04-complex/conv_compar_dof.png
   :align: center
   :scale: 45% 
   :figclass: align-center
   :alt: DOF convergence graph.
