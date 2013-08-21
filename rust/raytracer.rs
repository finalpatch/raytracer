use std::float;
use std::uint;
use std::vec;
use std::iterator;

struct Vec3 {
    x: float, 
    y: float,
    z: float
}
impl Vec3 {
    fn dump(&self) {
        println(fmt!("[%f %f %f]", self.x, self.y, self.z));
    }
    fn dot(&self, other: &Vec3) -> float {
        let t = (*self) * (*other);
        return t.x + t.y + t.z;
    }
    fn mag(&self) -> float {
        return float::sqrt(self.dot(self));
    }
    fn normalized(&self) -> Vec3 {
        return (*self) / s(self.mag());
    }
}
impl Add<Vec3, Vec3> for Vec3 {
    fn add(&self, other: &Vec3) -> Vec3 {
        Vec3 {x: self.x + other.x,
              y: self.y + other.y,
              z: self.z + other.z}
    }
}
impl Sub<Vec3, Vec3> for Vec3 {
    fn sub(&self, other: &Vec3) -> Vec3 {
        Vec3 {x: self.x - other.x,
              y: self.y - other.y,
              z: self.z - other.z}
    }
}
impl Mul<Vec3, Vec3> for Vec3 {
    fn mul(&self, other: &Vec3) -> Vec3 {
        Vec3 {x: self.x * other.x,
              y: self.y * other.y,
              z: self.z * other.z}
    }
}
impl Div<Vec3, Vec3> for Vec3 {
    fn div(&self, other: &Vec3) -> Vec3 {
        Vec3 {x: self.x / other.x,
              y: self.y / other.y,
              z: self.z / other.z}
    }
}
fn s (x: float) -> Vec3 {
    Vec3 { x: x, y: x, z: x }
}
fn v (x: float, y: float, z: float) -> Vec3 {
    Vec3 { x: x, y: y, z: z }
}

// **********************************************

struct Ray {
    start: Vec3,
    dir:   Vec3,
}

struct Sphere {
    center: Vec3,
    radius: float,
    color:  Vec3,
}

impl Sphere {
    fn normal (&self, pos: Vec3) -> Vec3 {
        return (pos - self.center).normalized();
    }
    fn intersect(&self, ray: &Ray) -> (bool, float) {
        let l = self.center - ray.start;
        let a = l.dot(&(ray.dir));
        if (a < 0.0f) {
            return (false, 0.0f);
        }
        let b2 = l.dot(&l) - a * a;
        let r2 = self.radius * self.radius;
        if (b2 > r2) {
            return (false, 0.0f);
        }
        let c = float::sqrt(r2 - b2);
        let near = a - c;
        let far = a + c;
        if (near < 0.0f) {
            (true, far)
        } else {
            (true, near)
        }
    }
}

struct Light {
    position: Vec3,
    color   : Vec3,
}

fn trace(ray: &Ray, scene: &[Sphere], depth: uint) -> Vec3 {
    let mut nearest = 1000000.0f;
    let mut hittest: Option<&Sphere> = None;
    for scene.iter().advance |o| {
        let result = o.intersect(ray);
        match result {
            (hit, distance) => {
                if (hit && distance < nearest) {
                    nearest = distance;
                    hittest = Some(o);
                }
            }
        }
    }
    match hittest {
        None => { return s(0.0f); }
        Some(obj) => {
            let point_of_hit = ray.start + ray.dir * s(nearest);
            let mut normal = obj.normal(point_of_hit);
            if (normal.dot(&ray.dir) > 0.0f) {
                normal = normal * s(-1.0f);
            }
            let mut color = s(0.0f);
            
            // light
            let l = v(-10.0, 20.0, 30.0);
            
            let light_direction=(l - point_of_hit).normalized();
            let r = Ray { start: point_of_hit + normal * s(0.0001f), dir: light_direction};
            let blocked = scene.iter().any_(|&o| {
                let result = o.intersect(&r);
                match result {
                    (success, _) => {success}
                }
            });
            if (!blocked)
            {
                color += s(2.0f)
                    * s(normal.dot(&light_direction).max(&0.0f))
                    * obj.color;
                return color;
            }
            else { s(0.0f) }
        }
    }
}

fn render(scene: &[Sphere], width: uint, height: uint) -> ~[Vec3] {
    let mut image = vec::from_elem(width*height, s(1.0f));
    let eye = s(0.0f);
    let fov = 45.0f;
    let h = float::tan(fov / 360.0f * 2.0f * 3.1415926535 / 2.0f) * 2.0f;
    let w = h * (width as float) / (height as float);

    for uint::range(0, height) |y| {
        for uint::range(0, width) |x| {
            let xx = x as float;
            let yy = y as float;
            let ww = width as float;
            let hh = height as float;
            let dir = v((xx - ww / 2.0f) / ww  * w,
                        (hh/2.0f - yy) / hh * h,
                        -1.0f).normalized();
            let ray = Ray { start: eye, dir: dir };
            image[y * width + x] = trace(&ray, scene, 0);
        }
    }

    return image;
}

fn main() {
    let scene: ~[Sphere] = ~[
        Sphere { center: v(0.0f, -10002.0f, -20.0f), radius: 10000.0f, color: v(0.8f, 0.8f, 0.8f) },
        Sphere { center: v(0.0f, 2.0f, -20.0f), radius: 4.0f, color: v(0.8f, 0.5f, 0.5f) },
        Sphere { center: v(5.0f, 0.0f, -15.0f), radius: 2.0f, color: v(0.3f, 0.8f, 0.8f) },
        Sphere { center: v(-5.0f, 0.0f, -15.0f), radius: 2.0f, color: v(0.3f, 0.5f, 0.8f) },
    ];
    let w = 1280;
    let h = 720;
    let image = render(scene, w, h);
    println(fmt!("P3\n%u\n%u\n255", w, h));
    for uint::range(0, w*h) |i| {
        match image[i] {
            Vec3 {x: r, y: g, z: b} => {
                println(fmt!("%d %d %d",
                             (r*255.0f).min(&255.0) as int,
                             (g*255.0f).min(&255.0) as int,
                             (b*255.0f).min(&255.0) as int));
            }
        }
    }
}
