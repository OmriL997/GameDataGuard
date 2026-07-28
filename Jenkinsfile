pipeline {
    agent any

    options {
        timeout(time: 30, unit: 'MINUTES')
        timestamps()
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Configure') {
            steps {
                script {
                    if (isUnix()) {
                        sh '''
                            cmake -B build \
                                -DCMAKE_BUILD_TYPE=Release \
                                -DGDG_WARNINGS_AS_ERRORS=ON
                        '''
                    } else {
                        bat '''
                            cmake -B build ^
                                -DCMAKE_BUILD_TYPE=Release ^
                                -DGDG_WARNINGS_AS_ERRORS=ON
                        '''
                    }
                }
            }
        }

        stage('Build') {
            steps {
                script {
                    if (isUnix()) {
                        sh 'cmake --build build --config Release --parallel'
                    } else {
                        bat 'cmake --build build --config Release --parallel'
                    }
                }
            }
        }

        stage('Test') {
            steps {
                script {
                    if (isUnix()) {
                        sh 'ctest --test-dir build --output-on-failure -C Release'
                    } else {
                        bat 'ctest --test-dir build --output-on-failure -C Release'
                    }
                }
            }
        }

        stage('Validate Sample Data') {
            steps {
                script {
                    if (isUnix()) {
                        sh '''
                            ./build/gamedataguard validate sample_data/valid \
                                --report validation_report.json
                        '''
                    } else {
                        bat '''
                            build\\Release\\gamedataguard.exe validate sample_data\\valid ^
                                --report validation_report.json
                        '''
                    }
                }
            }
        }

        stage('Package Sample Data') {
            steps {
                script {
                    if (isUnix()) {
                        sh './build/gamedataguard build sample_data/valid sample.gdgpack'
                    } else {
                        bat 'build\\Release\\gamedataguard.exe build sample_data\\valid sample.gdgpack'
                    }
                }
            }
        }

        stage('Archive Artifacts') {
            steps {
                archiveArtifacts artifacts: 'validation_report.json', allowEmptyArchive: true
                archiveArtifacts artifacts: 'sample.gdgpack', allowEmptyArchive: true
            }
        }
    }

    post {
        always {
            echo "Pipeline completed with status: ${currentBuild.result ?: 'SUCCESS'}"
        }
        failure {
            echo "Build failed. Check the logs above for details."
        }
    }
}
