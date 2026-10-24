//! Compile-time physical dimensional analysis using const generics.
//!
//! Guarantees that Effort * Flow = Power across all physical domains at compile time.
//! Zero runtime overhead (compiles directly to bare f64/f32 registers).

use core::ops::{Add, Sub, Mul, Div, Neg};

#[derive(Copy, Clone, Debug, PartialEq)]
pub struct Quantity<const M: i8, const L: i8, const T: i8, const I: i8, const THETA: i8>(pub f64);

/* Dimensionless number */
pub type Dimensionless = Quantity<0, 0, 0, 0, 0>;

/* Mechanical (Translational) */
pub type Force          = Quantity<1, 1, -2, 0, 0>; /* [N] */
pub type Velocity       = Quantity<0, 1, -1, 0, 0>; /* [m/s] */
pub type Displacement   = Quantity<0, 1, 0, 0, 0>;  /* [m] */
pub type LinearMomentum = Quantity<1, 1, -1, 0, 0>; /* [kg*m/s] */

/* Mechanical (Rotational) */
pub type Torque          = Quantity<1, 2, -2, 0, 0>; /* [N*m] */
pub type AngularVelocity = Quantity<0, 0, -1, 0, 0>; /* [rad/s] */
pub type Angle           = Quantity<0, 0, 0, 0, 0>;  /* [rad] (dimensionless) */
pub type AngularMomentum = Quantity<1, 2, -1, 0, 0>; /* [N*m*s] */

/* Electrical */
pub type Voltage     = Quantity<1, 2, -3, -1, 0>; /* [V] */
pub type Current     = Quantity<0, 0, 0, 1, 0>;   /* [A] */
pub type Charge      = Quantity<0, 0, 1, 1, 0>;   /* [C] */
pub type FluxLinkage = Quantity<1, 2, -2, -1, 0>; /* [Wb] */

/* Hydraulic */
pub type Pressure       = Quantity<1, -1, -2, 0, 0>; /* [Pa] */
pub type VolumetricFlow = Quantity<0, 3, -1, 0, 0>;  /* [m^3/s] */
pub type FluidVolume    = Quantity<0, 3, 0, 0, 0>;   /* [m^3] */

/* Thermal */
pub type Temperature = Quantity<0, 0, 0, 0, 1>;   /* [K] */
pub type EntropyFlow = Quantity<1, 2, -3, 0, -1>; /* [W/K] */
pub type Entropy     = Quantity<1, 2, -2, 0, -1>; /* [J/K] */

/* Unified Universal Currencies */
pub type Power  = Quantity<1, 2, -3, 0, 0>; /* [W] */
pub type Energy = Quantity<1, 2, -2, 0, 0>; /* [J] */

/* Arithmetic Operations */
impl<const M: i8, const L: i8, const T: i8, const I: i8, const THETA: i8>
    Quantity<M, L, T, I, THETA>
{
    pub const fn new(val: f64) -> Self {
        Self(val)
    }

    pub const fn value(self) -> f64 {
        self.0
    }
}

impl<const M: i8, const L: i8, const T: i8, const I: i8, const THETA: i8> Add
    for Quantity<M, L, T, I, THETA>
{
    type Output = Self;
    #[inline(always)]
    fn add(self, rhs: Self) -> Self::Output {
        Quantity(self.0 + rhs.0)
    }
}

impl<const M: i8, const L: i8, const T: i8, const I: i8, const THETA: i8> Sub
    for Quantity<M, L, T, I, THETA>
{
    type Output = Self;
    #[inline(always)]
    fn sub(self, rhs: Self) -> Self::Output {
        Quantity(self.0 - rhs.0)
    }
}

impl<const M: i8, const L: i8, const T: i8, const I: i8, const THETA: i8> Neg
    for Quantity<M, L, T, I, THETA>
{
    type Output = Self;
    #[inline(always)]
    fn neg(self) -> Self::Output {
        Quantity(-self.0)
    }
}

impl<const M: i8, const L: i8, const T: i8, const I: i8, const THETA: i8> Mul<f64>
    for Quantity<M, L, T, I, THETA>
{
    type Output = Self;
    #[inline(always)]
    fn mul(self, rhs: f64) -> Self::Output {
        Quantity(self.0 * rhs)
    }
}

impl<const M: i8, const L: i8, const T: i8, const I: i8, const THETA: i8> Div<f64>
    for Quantity<M, L, T, I, THETA>
{
    type Output = Self;
    #[inline(always)]
    fn div(self, rhs: f64) -> Self::Output {
        Quantity(self.0 / rhs)
    }
}

/* Compile-time dimensional multiplication: [M1+M2, L1+L2, T1+T2, I1+I2, THETA1+THETA2] */
pub trait DimensionMul<Rhs> {
    type Output;
    fn mul_dim(self, rhs: Rhs) -> Self::Output;
}

/* Universal Port Power Laws: Effort * Flow = Power across domains */
impl Mul<Velocity> for Force {
    type Output = Power;
    #[inline(always)]
    fn mul(self, rhs: Velocity) -> Self::Output {
        Quantity(self.0 * rhs.0)
    }
}

impl Mul<AngularVelocity> for Torque {
    type Output = Power;
    #[inline(always)]
    fn mul(self, rhs: AngularVelocity) -> Self::Output {
        Quantity(self.0 * rhs.0)
    }
}

impl Mul<Current> for Voltage {
    type Output = Power;
    #[inline(always)]
    fn mul(self, rhs: Current) -> Self::Output {
        Quantity(self.0 * rhs.0)
    }
}

impl Mul<VolumetricFlow> for Pressure {
    type Output = Power;
    #[inline(always)]
    fn mul(self, rhs: VolumetricFlow) -> Self::Output {
        Quantity(self.0 * rhs.0)
    }
}

impl Mul<EntropyFlow> for Temperature {
    type Output = Power;
    #[inline(always)]
    fn mul(self, rhs: EntropyFlow) -> Self::Output {
        Quantity(self.0 * rhs.0)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_power_unification() {
        let f = Force::new(10.0);
        let v = Velocity::new(2.5);
        let p_mech = f * v;
        assert_eq!(p_mech.value(), 25.0);

        let v_elec = Voltage::new(12.5);
        let i_elec = Current::new(2.0);
        let p_elec = v_elec * i_elec;
        assert_eq!(p_elec.value(), 25.0);

        let press = Pressure::new(250.0);
        let flow = VolumetricFlow::new(0.1);
        let p_hyd = press * flow;
        assert_eq!(p_hyd.value(), 25.0);

        let temp = Temperature::new(300.0);
        let s_dot = EntropyFlow::new(0.5);
        let p_therm = temp * s_dot;
        assert_eq!(p_therm.value(), 150.0);
    }
}
