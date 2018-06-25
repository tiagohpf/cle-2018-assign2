#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

// Definition of functions
int insert_measure(char *measure);
long insert_frequency();
void initialize_array(char ***nodes, int W, int D, int H);
int menu();
void insert_coordinates(float *x, float *y, float *z);
void insert_max_min_coordinates(float *min, float *max, char letter);
float insert_radius();
float insert_absortion_coefficient();
char coefficient_option();
void add_new_source_receiver(char ***nodes, int W, int D, int H, float d, char *element);
void add_new_sphere(char ***nodes, int W, int D, int H, float d);
void add_new_cuboid(char ***nodes, int W, int D, int H, float d);
void save_data(char ***nodes, int W, int D, int H, long fu);

int main() {
  int W, D, H, sources = 0, receivers = 0;
  long fu;
  float x, y, z, xmin, xmax, ymin, ymax, zmin, zmax;
  char ***nodes;
  const int c = 344;

  W = insert_measure("Width");
  D = insert_measure("Depth");
  H = insert_measure("Height");
  fu = insert_frequency();
  float node_spacing = (c * sqrt(3)) / fu;
  W = W / node_spacing;
  D = D / node_spacing;
  H = H / node_spacing;
  int Wl = (int) ceil(node_spacing * W);
  int Dl = (int) ceil(node_spacing * D);
  int Hl = (int) ceil(node_spacing * H);
  printf("Adjustment of room\n");
  printf("W = %d m, D = %d m, H = %d m\n", W, D, H);
  printf("W' = %d, D' = %d, H' = %d with d = %f\n", Wl, Dl, Hl, node_spacing);

  nodes = (char***) malloc(W * sizeof(char**));
  initialize_array(nodes, W, D, H);

  int op = -1;
  do {
    op = menu();
    switch (op) {
      case 1:
        printf("Sphere\n");
        add_new_sphere(nodes, W, D, H, node_spacing);
        break;
      case 2:
        printf("Cuboid\n");
        add_new_cuboid(nodes, W, D, H, node_spacing);
        break;
      case 3:
        printf("Source\n");
        add_new_source_receiver(nodes, W, D, H, node_spacing, "Source");
        sources++;
        break;
      case 4:
        printf("Receiver\n");
        add_new_source_receiver(nodes, W, D, H, node_spacing, "Receiver");
        receivers++;
        break;
      case 5:
        printf("Save\n");
        if (sources < 1 || receivers < 1)
          printf("It must be specified at least one source and one receiver\n");
        save_data(nodes, W, D, H, fu);
        break;
      case 0:
        printf("Finishing program.\n");
        break;
      default:
        printf("Invalid option.\n");
        break;
    }
  } while(op != 0);
}

int insert_measure(char *measure) {
  int num;
  do {
    printf("%s:", measure);
    scanf("%d", &num);
    if (num <= 0)
      printf("%s must be a positive number.\n", measure);
  } while(num <= 0);
  return num;
}

long insert_frequency() {
  int op;
  long frequency;
  do {
    printf("Choose a Frequency\n");
    printf("1. 48000 Hz\n");
    printf("2. 44100 Hz\n");
    printf("3. 22050 Hz\n");
    printf("4. 16000 Hz\n");
    printf("5. 11025 Hz\n");
    printf("6. 8000 Hz\n");
    printf("Option: ");
    scanf("%d", &op);
    if (op < 1 || op > 6)
      printf("Invalid option.\n");
  } while(op < 1 || op > 6);

  switch (op) {
    case 1:
      frequency = 48000;
      break;
    case 2:
      frequency = 44100;
      break;
    case 3:
      frequency = 22050;
      break;
    case 4:
      frequency = 16000;
      break;
    case 5:
      frequency = 11025;
      break;
    case 6:
      frequency = 8000;
      break;
  }
  return frequency;
}

void initialize_array(char*** nodes, int W, int D, int H) {
  for (int x = 0; x < W; x++) {
    nodes[x] = (char**) malloc(D * sizeof (char*));
    for (int y = 0; y < D; y++) {
      nodes[x][y] = (char*) malloc(H * sizeof(char));
      for (int z = 0; z < H; z++)
        nodes[x][y][z] = ' ';
    }
  }
}

int menu() {
  int op;
  printf("*** MENU ***\n");
  printf("1. Add Sphere\n");
  printf("2. Add Cuboid\n");
  printf("3. Add Source\n");
  printf("4. Add Receiver\n");
  printf("5. Save Configurations\n");
  printf("0. Exit\n");
  printf("Option: ");
  scanf("%d", &op);
  return op;
}

void insert_coordinates(float *x, float *y, float *z) {
  printf("x: ");
  scanf("%f", x);
  printf("y: ");
  scanf("%f", y);
  printf("z: ");
  scanf("%f", z);
}

float insert_radius() {
  float r = -1;
  do {
    printf("Radius: ");
    scanf("%f", &r);
    if (r < 0)
      printf("Radius must be zero or positive.\n");
  } while(r < 0);
  return r;
}

float insert_absortion_coefficient() {
  char op;
  bool correct = false;
  double coefficient;
  do {
    op = coefficient_option();
    switch(op) {
      case 'A':
        coefficient = 0;
        correct = true;
        break;
      case 'B':
        coefficient = 0.1;
        correct = true;
        break;
      case 'C':
        coefficient = 0.2;
        correct = true;
        break;
      case 'D':
        coefficient = 0.3;
        correct = true;
        break;
      case 'E':
        coefficient = 0.4;
        correct = true;
        break;
      case 'F':
        coefficient = 0.5;
        correct = true;
        break;
      case 'G':
        coefficient = 0.6;
        correct = true;
        break;
      case 'H':
        coefficient = 0.7;
        correct = true;
        break;
      case 'I':
        coefficient = 0.8;
        correct = true;
        break;
      case 'J':
        coefficient = 0.9;
        correct = true;
        break;
      case '1':
        coefficient = 0.91;
        correct = true;
        break;
      case '2':
        coefficient = 0.92;
        correct = true;
        break;
      case '3':
        coefficient = 0.93;
        correct = true;
        break;
      case '4':
        coefficient = 0.94;
        correct = true;
        break;
      case '5':
        coefficient = 0.95;
        correct = true;
        break;
      case '6':
        coefficient = 0.96;
        correct = true;
        break;
      case '7':
        coefficient = 0.97;
        correct = true;
        break;
      case '8':
        coefficient = 0.98;
        correct = true;
        break;
      case '9':
        coefficient = 0.99;
        correct = true;
        break;
      case 'Z':
        coefficient = 1;
        correct = true;
        break;
      default:
        printf("Invalid Option.\n");
        correct = false;
        break;
    }
  } while(correct == false);
  return coefficient;
}

char coefficient_option() {
  char op;
  printf("A. Fully Absorptive (p = 0)\n");
  printf("B. p = 0.1\n");
  printf("C. p = 0.2\n");
  printf("D. p = 0.3\n");
  printf("E. p = 0.4\n");
  printf("F. p = 0.5\n");
  printf("G. p = 0.6\n");
  printf("H. p = 0.7\n");
  printf("I. p = 0.8\n");
  printf("J. p = 0.9\n");
  printf("1. p = 0.91\n");
  printf("2. p = 0.92\n");
  printf("3. p = 0.93\n");
  printf("4. p = 0.94\n");
  printf("5. p = 0.95\n");
  printf("6. p = 0.96\n");
  printf("7. p = 0.97\n");
  printf("8. p = 0.98\n");
  printf("9. p = 0.99\n");
  printf("Z. Fully Reflective (p = 1)\n");
  printf("Option: ");
  getchar();
  scanf("%c", &op);
  return op;
}

void add_new_source_receiver(char ***nodes, int W, int D, int H, float d, char *element) {
  float x, y, z;
  insert_coordinates(&x, &y, &z);
  // Adjust Values
  int i  = (int) x / d;
  int j = (int) y / d;
  int k = (int) z / d;
  if (element == "Source") {
    nodes[i][j][k] = 'S';
    printf("New Source Added in (%d, %d, %d)\n", i, j, k);
  }
  else if (element == "Receiver") {
    nodes[i][j][k] = 'R';
    printf("New Receiver Added in (%d, %d, %d)\n", i, j, k);
  }
}

void add_new_sphere(char ***nodes, int W, int D, int H, float d) {
  float x, y, z, r, coefficient;
  insert_coordinates(&x, &y, &z);
  r = insert_radius();
  coefficient = insert_absortion_coefficient();

  // Adjust Values
  float i = (int) x / d;
  float j = (int) y / d;
  float k = (int) z / d;
  for (int a = 0; a < W; a++) {
    for (int b = 0; b < D; b++) {
      for (int c = 0; c < H; c++) {
        float a_converted = a * d + (d / 2);
        float b_converted = b * d + (d / 2);
        float c_converted = c * d + (d / 2);
        if ((pow((i - a_converted), 2) + pow((j - b_converted), 2) + pow((k - c_converted), 2)) <= pow(r, 2))
          nodes[a][b][c] = coefficient;
      }
    }
  }
  printf("New Sphere Added\n");
}

void add_new_cuboid(char ***nodes, int W, int D, int H, float d) {
  float x_min = 0, x_max = 0, y_min = 0, y_max = 0, z_min = 0, z_max = 0, coefficient;
  printf("Set [min, max]\n");
  insert_max_min_coordinates(&x_min, &x_max, 'x');
  insert_max_min_coordinates(&y_min, &y_max, 'y');
  insert_max_min_coordinates(&z_min, &z_max, 'z');
  coefficient = insert_absortion_coefficient();

  // Adjust Values
  float i_min = (int) x_min / d;
  float i_max = (int) x_max / d;
  float j_min = (int) y_min / d;
  float j_max = (int) y_max / d;
  float k_min = (int) z_min / d;
  float k_max = (int) z_max / d;

  for (int a = 0; a < W; a++) {
    for (int b = 0; b < D; b++) {
      for (int c = 0; c < H; c++) {
        float a_converted = a * d + (d / 2);
        float b_converted = b * d + (d / 2);
        float c_converted = c * d + (d / 2);
        if (a_converted >= i_min && a_converted <= i_max
            && b_converted >= j_min && b_converted <= j_max
            && c_converted >= k_min && c_converted <= k_max)
            nodes[a][b][c] = coefficient;
      }
    }
  }
  printf("New Cuboid Added\n");
}

void insert_max_min_coordinates(float *min, float *max, char letter) {
  do {
    printf("%c min: ", letter);
    scanf("%f", min);
    printf("%c max: ", letter);
    scanf("%f", max);
    if (*min >= *max)
      printf("Min must be lower than Max\n");
  } while (*min >= *max);
  printf("%c: [%f, %f]\n", letter, *min, *max);
}

void save_data(char ***nodes, int W, int D, int H, long fu) {
  FILE *file = fopen("room-config.dwm", "w");
  int number = 0;
  for (int i = 0; i < W; i++) {
    for (int j = 0; j < D; j++) {
      for (int k = 0; k < H; k++) {
        number++;
      }
    }
  }
  printf("%d\n", number);
  fprintf(file, "%d %d %d %ld\n", W, D, H, fu);
  for (int i = 0; i < W; i++) {
    for (int j = 0; j < D; j++) {
      for (int k = 0; k < H; k++) {
        fprintf(file, "%c", nodes[i][j][k]);
      }
    }
  }
  fclose(file);
  printf("File Saved\n");
}
