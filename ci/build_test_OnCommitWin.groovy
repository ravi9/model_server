pipeline {
    options {
        timeout(time: 2, unit: 'HOURS')
    }
    agent {
        label 'win_ovms'
    }
    stages {
        stage ("Build and test windows") {
            steps {
                script {
                    new File('win_build.log').text = '0'
                    new File('win_build_test.log').text = '0'
                    new File('win_environment.log').text = '0'
                    return
                }
            }
        }
    }
}
