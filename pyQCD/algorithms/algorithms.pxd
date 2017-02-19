from pyQCD.core cimport atomics, core
from pyQCD.gauge cimport gauge
from pyQCD.fermions cimport fermions

cdef extern from "algorithms/heatbath.hpp" namespace "pyQCD":
    cdef void _heatbath_update "pyQCD::heatbath_update"(
        core._LatticeColourMatrix&,
        const gauge._GaugeAction&, const unsigned int)

cdef extern from "algorithms/types.hpp" namespace "pyQCD::python":
    cdef cppclass _SolutionWrapper "pyQCD::python::SolutionWrapper":
        _SolutionWrapper(_SolutionWrapper&&)
        const core._LatticeColourVector& solution() const
        atomics.Real tolerance() const
        unsigned int num_iterations() const

cdef extern from "conjugate_gradient.hpp" namespace "pyQCD":
    cdef _SolutionWrapper _conjugate_gradient "pyQCD::conjugate_gradient"(
        const fermions._FermionAction&, const core._LatticeColourVector&,
        const unsigned int, const atomics.Real)