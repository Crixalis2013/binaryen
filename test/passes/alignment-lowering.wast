(module
 (memory $0 1 1)
 (func $func_4
  (drop (i32.load (i32.const 4)))
  (drop (i32.load align=1 (i32.const 4)))
  (drop (i32.load align=2 (i32.const 4)))
  (drop (i32.load align=4 (i32.const 4)))
  (drop (i32.load offset=100 (i32.const 4)))
  (drop (i32.load offset=100 align=1 (i32.const 4)))
  (drop (i32.load offset=100 align=2 (i32.const 4)))
  (drop (i32.load offset=100 align=4 (i32.const 4)))
  (drop (i32.load offset=100 align=1 (unreachable)))
  (i32.store (i32.const 4) (i32.const 8))
  (i32.store align=1 (i32.const 4) (i32.const 8))
  (i32.store align=2 (i32.const 4) (i32.const 8))
  (i32.store align=4 (i32.const 4) (i32.const 8))
  (i32.store offset=100 (i32.const 4) (i32.const 8))
  (i32.store offset=100 align=1 (i32.const 4) (i32.const 8))
  (i32.store offset=100 align=2 (i32.const 4) (i32.const 8))
  (i32.store offset=100 align=4 (i32.const 4) (i32.const 8))
  (i32.store offset=100 align=1 (unreachable) (i32.const 8))
  (i32.store offset=100 align=1 (i32.const 4) (unreachable))
 )
 (func $func_2
  (drop (i32.load16_u (i32.const 4)))
  (drop (i32.load16_u align=1 (i32.const 4)))
  (drop (i32.load16_u align=2 (i32.const 4)))
  (drop (i32.load16_u offset=100 (i32.const 4)))
  (drop (i32.load16_u offset=100 align=1 (i32.const 4)))
  (drop (i32.load16_u offset=100 align=2 (i32.const 4)))
  (drop (i32.load16_u offset=100 align=1 (unreachable)))
  (i32.store16 (i32.const 4) (i32.const 8))
  (i32.store16 align=1 (i32.const 4) (i32.const 8))
  (i32.store16 align=2 (i32.const 4) (i32.const 8))
  (i32.store16 offset=100 (i32.const 4) (i32.const 8))
  (i32.store16 offset=100 align=1 (i32.const 4) (i32.const 8))
  (i32.store16 offset=100 align=2 (i32.const 4) (i32.const 8))
  (i32.store16 offset=100 align=1 (unreachable) (i32.const 8))
  (i32.store16 offset=100 align=1 (i32.const 4) (unreachable))
 )
 (func $func_1
  (drop (i32.load8_u (i32.const 4)))
  (drop (i32.load8_u align=1 (i32.const 4)))
  (drop (i32.load8_u offset=100 (i32.const 4)))
  (drop (i32.load8_u offset=100 align=1 (i32.const 4)))
  (drop (i32.load8_u offset=100 align=1 (unreachable)))
  (i32.store8 (i32.const 4) (i32.const 8))
  (i32.store8 align=1 (i32.const 4) (i32.const 8))
  (i32.store8 offset=100 (i32.const 4) (i32.const 8))
  (i32.store8 offset=100 align=1 (i32.const 4) (i32.const 8))
  (i32.store8 offset=100 align=1 (unreachable) (i32.const 8))
  (i32.store8 offset=100 align=1 (i32.const 4) (unreachable))
 )
 (func $func_signed
  (drop (i32.load16_s (i32.const 4)))
  (drop (i32.load16_s align=1 (i32.const 4)))
  (drop (i32.load16_s align=2 (i32.const 4)))
  (drop (i32.load16_s offset=100 (i32.const 4)))
  (drop (i32.load16_s offset=100 align=1 (i32.const 4)))
  (drop (i32.load16_s offset=100 align=2 (i32.const 4)))
  (drop (i32.load16_s offset=100 align=1 (unreachable)))
 )
 (func $i64-load
  (drop (i64.load align=1 (i32.const 12)))
  (drop (i64.load align=2 (i32.const 16)))
  (drop (i64.load align=4 (i32.const 20)))
 )
 (func $f32-load
  (drop (f32.load align=1 (i32.const 12)))
  (drop (f32.load align=2 (i32.const 16)))
 )
 (func $f64-load
  (drop (f64.load align=1 (i32.const 12)))
  (drop (f64.load align=2 (i32.const 16)))
  (drop (f64.load align=4 (i32.const 20)))
 )
)
