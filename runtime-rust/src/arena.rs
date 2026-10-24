//! Const-generic, zero-heap static memory arena for bare-metal `#![no_std]` targets.

#[derive(Debug)]
pub struct StaticArena<const CAPACITY: usize> {
    storage: [u8; CAPACITY],
    offset: usize,
    high_water_mark: usize,
}

impl<const CAPACITY: usize> StaticArena<CAPACITY> {
    pub const fn new() -> Self {
        Self {
            storage: [0; CAPACITY],
            offset: 0,
            high_water_mark: 0,
        }
    }

    pub fn alloc<T: Copy + Default>(&mut self, count: usize) -> Option<&'static mut [T]> {
        let align = core::mem::align_of::<T>();
        let size = core::mem::size_of::<T>() * count;

        let current_ptr = self.storage.as_ptr() as usize + self.offset;
        let aligned_ptr = (current_ptr + (align - 1)) & !(align - 1);
        let padding = aligned_ptr - current_ptr;

        if self.offset + padding + size > CAPACITY {
            return None;
        }

        self.offset += padding + size;
        if self.offset > self.high_water_mark {
            self.high_water_mark = self.offset;
        }

        let slice = unsafe {
            core::slice::from_raw_parts_mut(aligned_ptr as *mut T, count)
        };
        for item in slice.iter_mut() {
            *item = T::default();
        }
        Some(slice)
    }

    pub fn reset(&mut self) {
        self.offset = 0;
    }

    pub fn mark(&self) -> usize {
        self.offset
    }

    pub fn restore(&mut self, mark: usize) {
        if mark <= CAPACITY {
            self.offset = mark;
        }
    }

    pub fn high_water_mark(&self) -> usize {
        self.high_water_mark
    }
}
