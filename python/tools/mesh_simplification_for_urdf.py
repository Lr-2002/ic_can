import os
import open3d as o3d

FOLDER = "/Users/lr-2002/project/instantcreation/ic_can/urdf/urdf"
MAX_FACES = 200_000

for fname in os.listdir(FOLDER):
    if fname.lower().endswith(".stl"):
        path = os.path.join(FOLDER, fname)
        print(f"\n🧩 处理文件: {fname}")
        try:
            mesh = o3d.io.read_triangle_mesh(path)
            num_faces = len(mesh.triangles)
            print(f"  当前面数: {num_faces}")

            if num_faces > MAX_FACES:
                ratio = MAX_FACES / num_faces
                print(f"  ⏬ 超出上限, 将降采样到 {ratio:.2%} ({MAX_FACES} 面)")
                simplified = mesh.simplify_quadric_decimation(
                    target_number_of_triangles=MAX_FACES
                )
                simplified.compute_vertex_normals()  # ✅ 修复关键行
                out_path = os.path.join(
                    FOLDER, fname.replace(".stl", "_simplified.stl")
                )
                o3d.io.write_triangle_mesh(out_path, simplified, write_ascii=False)
                print(f"  ✅ 已输出: {out_path}")
            else:
                print("  ✅ 面数低于 20 万，无需简化。")
        except Exception as e:
            print(f"  ⚠️ 处理失败: {e}")
