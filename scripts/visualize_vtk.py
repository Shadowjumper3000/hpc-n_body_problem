#!/usr/bin/env python3
"""
Simple VTK file visualization and movie generation
Usage: python visualize_vtk.py [vtk_file_or_directory]
"""

import sys
import os
import glob

try:
    import vtk
    from vtk.util import numpy_support
    import numpy as np
except ImportError:
    print("Error: VTK not installed")
    print("Install with: pip install vtk")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
    HAS_MPL = True
except ImportError:
    print("Warning: matplotlib not found, using VTK renderer only")
    HAS_MPL = False


def read_vtk_file(filename):
    """Read VTK polydata file"""
    reader = vtk.vtkPolyDataReader()
    reader.SetFileName(filename)
    reader.Update()
    polydata = reader.GetOutput()

    # Extract points
    points = polydata.GetPoints()
    n_points = points.GetNumberOfPoints()

    positions = np.zeros((n_points, 3))
    for i in range(n_points):
        positions[i] = points.GetPoint(i)

    # Extract velocities if available
    velocities = None
    if polydata.GetPointData().GetArray('velocity'):
        vel_array = polydata.GetPointData().GetArray('velocity')
        velocities = numpy_support.vtk_to_numpy(vel_array)

    # Extract masses if available
    masses = None
    if polydata.GetPointData().GetScalars('mass'):
        mass_array = polydata.GetPointData().GetScalars('mass')
        masses = numpy_support.vtk_to_numpy(mass_array)

    return positions, velocities, masses


def plot_particles_matplotlib(positions, velocities=None, masses=None, output_file=None):
    """Plot particles using matplotlib"""
    if not HAS_MPL:
        print("Matplotlib not available")
        return

    fig = plt.figure(figsize=(12, 10))
    ax = fig.add_subplot(111, projection='3d')

    # Color by mass or velocity magnitude
    if velocities is not None:
        colors = np.linalg.norm(velocities, axis=1)
        label = 'Velocity magnitude'
    elif masses is not None:
        colors = masses
        label = 'Mass'
    else:
        colors = 'blue'
        label = None

    scatter = ax.scatter(positions[:, 0], positions[:, 1], positions[:, 2],
                        c=colors, s=1, alpha=0.6, cmap='viridis')

    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title(f'N-Body Simulation ({len(positions)} particles)')

    if label:
        cbar = plt.colorbar(scatter, ax=ax, pad=0.1)
        cbar.set_label(label)

    # Equal aspect ratio
    max_range = np.max(np.ptp(positions, axis=0)) / 2.0
    mid_x = np.mean(positions[:, 0])
    mid_y = np.mean(positions[:, 1])
    mid_z = np.mean(positions[:, 2])
    ax.set_xlim(mid_x - max_range, mid_x + max_range)
    ax.set_ylim(mid_y - max_range, mid_y + max_range)
    ax.set_zlim(mid_z - max_range, mid_z + max_range)

    if output_file:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Saved: {output_file}")
    else:
        plt.show()

    plt.close()


def create_movie(vtk_files, output_file='nbody_movie.mp4'):
    """Create movie from VTK files using matplotlib"""
    if not HAS_MPL:
        print("Matplotlib required for movie generation")
        return

    try:
        from matplotlib.animation import FFMpegWriter
    except ImportError:
        print("FFMpeg not available for movie generation")
        return

    print(f"Creating movie from {len(vtk_files)} frames...")

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    # Read first frame to set up plot
    positions, velocities, masses = read_vtk_file(vtk_files[0])

    # Determine global bounds
    all_positions = []
    for vtkfile in vtk_files[:min(10, len(vtk_files))]:  # Sample for bounds
        pos, _, _ = read_vtk_file(vtkfile)
        all_positions.append(pos)
    all_positions = np.vstack(all_positions)

    max_range = np.max(np.ptp(all_positions, axis=0)) / 2.0
    mid_x = np.mean(all_positions[:, 0])
    mid_y = np.mean(all_positions[:, 1])
    mid_z = np.mean(all_positions[:, 2])

    # Set up writer
    writer = FFMpegWriter(fps=10)

    with writer.saving(fig, output_file, dpi=100):
        for i, vtkfile in enumerate(vtk_files):
            print(f"Processing frame {i+1}/{len(vtk_files)}...", end='\r')

            positions, velocities, masses = read_vtk_file(vtkfile)

            ax.clear()

            if velocities is not None:
                colors = np.linalg.norm(velocities, axis=1)
            else:
                colors = 'blue'

            ax.scatter(positions[:, 0], positions[:, 1], positions[:, 2],
                      c=colors, s=1, alpha=0.5, cmap='viridis')

            ax.set_xlabel('X')
            ax.set_ylabel('Y')
            ax.set_zlabel('Z')
            ax.set_title(f'N-Body Simulation (Frame {i+1}/{len(vtk_files)})')

            ax.set_xlim(mid_x - max_range, mid_x + max_range)
            ax.set_ylim(mid_y - max_range, mid_y + max_range)
            ax.set_zlim(mid_z - max_range, mid_z + max_range)

            writer.grab_frame()

    print(f"\nMovie saved: {output_file}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python visualize_vtk.py <vtk_file or directory>")
        print("\nExamples:")
        print("  python visualize_vtk.py results/output_0.vtk")
        print("  python visualize_vtk.py results/  # Process all VTK files")
        sys.exit(1)

    path = sys.argv[1]

    if os.path.isfile(path):
        # Single file
        print(f"Reading: {path}")
        positions, velocities, masses = read_vtk_file(path)
        print(f"Particles: {len(positions)}")

        output_file = path.replace('.vtk', '.png')
        plot_particles_matplotlib(positions, velocities, masses, output_file)

    elif os.path.isdir(path):
        # Directory - process all VTK files
        vtk_files = sorted(glob.glob(os.path.join(path, '*.vtk')))

        if not vtk_files:
            print(f"No VTK files found in {path}")
            sys.exit(1)

        print(f"Found {len(vtk_files)} VTK files")

        # Create images for each
        output_dir = os.path.join(path, 'images')
        os.makedirs(output_dir, exist_ok=True)

        for vtkfile in vtk_files:
            basename = os.path.splitext(os.path.basename(vtkfile))[0]
            output_file = os.path.join(output_dir, f"{basename}.png")

            positions, velocities, masses = read_vtk_file(vtkfile)
            plot_particles_matplotlib(positions, velocities, masses, output_file)

        # Create movie
        movie_file = os.path.join(path, 'nbody_simulation.mp4')
        create_movie(vtk_files, movie_file)

    else:
        print(f"Error: {path} not found")
        sys.exit(1)


if __name__ == '__main__':
    main()
