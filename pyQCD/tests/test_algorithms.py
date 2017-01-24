from __future__ import absolute_import

import pytest

from pyQCD import algorithms, core, gauge


@pytest.fixture
def gauge_field():
    """Generate a cold-start gauge field with the supplied lattice shape"""
    shape = (8, 4, 4, 4)
    gauge_field = core.LatticeColourMatrix(shape, len(shape))
    gauge_field.as_numpy.fill(0.0)

    for i in range(gauge_field.as_numpy.shape[-1]):
        gauge_field.as_numpy[..., i, i] = 1.0

    return gauge_field


@pytest.fixture
def action():
    """Create an instance of the Wilson gauge action"""
    shape = (8, 4, 4, 4)
    return gauge.WilsonGaugeAction(5.5, shape)

def test_heatbath_update(action, gauge_field):
    """Test heatbath_update method"""
    algorithms.heatbath_update(gauge_field, action, 1)