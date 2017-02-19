from pyQCD.core.core cimport LatticeColourMatrix, LatticeColourVector
from pyQCD.fermions.fermions cimport FermionAction
from pyQCD.gauge.gauge cimport GaugeAction

from algorithms cimport _heatbath_update


def heatbath_update(LatticeColourMatrix gauge_field,
                    GaugeAction action, int num_updates):
    _heatbath_update(gauge_field.instance[0], action.instance[0], num_updates)


def conjugate_gradient(FermionAction action, LatticeColourVector rhs,
                       int max_iterations, core.Real tolerance):
    cdef _SolutionWrapper* wrapped_solution =\
    new _SolutionWrapper(_conjugate_gradient(
        action.instance[0], rhs.instance[0], max_iterations, tolerance))

    cdef int num_dims = rhs.instance.num_dims()

    solution = LatticeColourVector(rhs.shape[:num_dims], rhs.shape[num_dims])
    solution.instance[0] = wrapped_solution.solution()

    return (solution, wrapped_solution.num_iterations(),
            wrapped_solution.tolerance())